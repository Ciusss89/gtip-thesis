/**
 * Author:	Giuseppe Tipaldi
 * Created:	2020
 **/
#define _GNU_SOURCE

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

/* RIOT APIs */
#include "thread.h"
#include "xtimer.h"
#include "board.h"
#include "msg.h"

/* RIOT APIs */
#include "periph/cpuid.h"

/* RIOT CAN APIs */
#include "can/conn/raw.h"
#include "can/device.h"
#include "can/can.h"

#define ENABLE_DEBUG    (0)
#include "debug.h"

#include "ihb-tools/tools.h"
#include "can.h"
#include "ihb.h"

#define RCV_TIMEOUT	(2000U * US_PER_MS)	/* socket rcv timeout */

/* ASCII message "IHB-ID" : ihb discovery message sent on CAN bus */
static const unsigned char mgc[] = {0x49, 0x48, 0x42, 0x2D, 0x49, 0x44, 0};
/* ASCII message "IHB-" : receive new address by ihbtool, runtime fix */
static unsigned char add_fix[8] = {0x49, 0x48, 0x42, 0x2D, 0x0, 0x0, 0x3D, 0x0};
/* ASCII message "IHB-WKUP" : switch the node in notify state */
static const unsigned char wkup[] = {0x49, 0x48, 0x42, 0x2D, 0x57, 0x4B, 0x55, 0x50, 0};
/* ASCII message "IHB-MSTR" : switch the node in action state */
static const unsigned char mstr[] = {0x49, 0x48, 0x42, 0x2D, 0x4D, 0x53, 0x54, 0x52, 0};
/* ASCII message "IHB-BCKP" : switch the node in backup state */
static const unsigned char bckp[] = {0x49, 0x48, 0x42, 0x2D, 0x42, 0x43, 0x4B, 0x50, 0};

static char can_handler_node_stack[THREAD_STACKSIZE_MEDIUM];

/* PID which generate the data which have to sent by isotp */
static kernel_pid_t pid_of_data_source;

static struct ihb_node_info *info = NULL;
static struct ihb_can_ctx *can = NULL;
static uint8_t LSBytes[2];

static int  _scan_for_controller(struct ihb_can_ctx *device)
{
	const char *raw_can = raw_can_get_name_by_ifnum(CAN_C);

	if (raw_can && strlen(raw_can) < CAN_NAME_LEN) {
		device->can_perh_id = CAN_C;
		strcpy(device->can_drv_name, raw_can);
		DEBUG("[#] CAN controller=%d, name=%s\n", device->can_perh_id, device->can_drv_name);
		return 0;
	}

	return 1;
}

static void _start_isotp_tx(void)
{
	/* Open the iso-tp socket */
	ihb_isotp_init(can->can_perh_id,
		       ISOTP_TIMEOUT_DEF,
		       &(can->can_isotp_ready));

	/* Send the ihb-info as first chunks */
	if (ihb_isotp_send_chunks(info, sizeof(struct ihb_node_info), 1) > 0) {
		free(info);
		can->can_isotp_ready = true;
		thread_wakeup(pid_of_data_source);
		puts("[*] ihb: node is sending data");
	} else {
		/* !TODO: Handle this patch code */
		ihb_isotp_close();
	}
}

static bool _raw_init(conn_can_raw_t *socket,
		      struct can_frame *frame,
		      struct can_filter *filter)
{
	int r;

	r = conn_can_raw_create(socket, NULL, 0, can->can_perh_id, 0);
	if (r < 0) {
		printf("[!] cannot create the CAN socket: err=%d\n", r);
		return false;
	}

	/* Configure the frame which has to be send */
	frame->can_id = can->can_frame_id;
	frame->can_dlc = 8;

	/* Last two bytes of message are the LSBytes of MCU unique id */
	memcpy(frame->data, &mgc, 6);
	frame->data[6] = LSBytes[0];
	frame->data[7] = LSBytes[1];
	
	/* Setup the filter for rcv socket */
	filter->can_mask = CAN_SFF_MASK | CAN_RTR_FLAG;

	return true;
}

static void print_raw_frame(struct can_frame *rcv_frame)
{
	if (!ENABLE_DEBUG)
		return;

	printf("[#] ihbcan: ID=%"PRIu32"  DLC=[%u] DATA=", rcv_frame->can_id,
							   rcv_frame->can_dlc);
	for (int i = 0; i < rcv_frame->can_dlc; i++)
		printf(" %#x", rcv_frame->data[i]);
	puts("");
}

static void _raw_frame_analize(struct can_frame *frame)
{
	if (state_is(NOTIFY) || state_is(BACKUP)) {

		/* Is it master ? */
		if (memcmp(&mstr, frame->data, frame->can_dlc) == 0) {
			state_event(MASTER);
			/* TODO: send ack to ihbtool */
			return;
		}

		/* Is it slave ? */
		if (memcmp(&bckp, frame->data, frame->can_dlc) == 0) {
			state_event(SLAVE);
			/* TODO: send ack to ihbtool */
			return;
		}

		/*
		 *                      ┌ New CAN frame id
		 * ASCI Message: IHB-XX=Y
		 *                   ||
		 *        LSBytes[0] ┘└ LSBytes[1]
		 *
		 * Fix runtime the addressing, last byte contains the new
		 * frame identifier which must be used
		 */
		if (memcmp(&add_fix, frame->data, 7) == 0) {
			printf("[*] IHB CAN id received, NEW=%#x, OLD=%#x\n",
					frame->data[7],
					can->can_frame_id);
			if (frame->data[4] == LSBytes[0] &&
			    frame->data[5] == LSBytes[1])
				can->can_frame_id = frame->data[7];
			state_event(RUNT_FIX);
			/* TODO: send ack to ihbtool */
			return;
		}

	} else if (state_is(IDLE)) {

		/*
		 * IHBTOOL to wake up an IHB have to send a broadcast message
		 * with frame id IHBTOLL_FRAME_ID
		 */
		if (memcmp(&wkup, frame->data, frame->can_dlc) == 0 &&
		    frame->can_id == IHBTOLL_FRAME_ID) {
			state_event(WAKEUP);
			return;
		}
	}
}

static int _raw_rcv_set_filter(conn_can_raw_t *socket, struct can_filter *filter)
{
	int r;

	if (state_is(IDLE))
		filter->can_id = IHBTOLL_FRAME_ID;
	else if (state_is(NOTIFY))
		filter->can_id = can->can_frame_id;

	/* Set hw filer to receive frame which have my CAN frame ID */
	r = conn_can_raw_set_filter(socket, filter, 1);
	if (r < 0) {
		printf("[!] cannot add the filter to the socket: err=%d", r);
		return r;
	}

	return 0;
}

static int _raw_frame_snd(conn_can_raw_t *socket, struct can_frame *frame)
{
	int r;

	/* Remove hw filter */
	r = conn_can_raw_set_filter(socket, NULL, 0);
	if (r < 0) {
		printf("[!] cannot remove filter from socket: err=%d", r);
		state_event(FAIL);
		return r;
	}

	/* Send */
	r = conn_can_raw_send(socket, frame, 0);
	if (r < 0) {
		printf("[!] error sending the CAN frame: err=%d\n", r);
		state_event(FAIL);
		return r;
	}

	return 0;
}

static void _add_can_info(void)
{
	strncpy(info->mcu_uid, can->mcu_controller_uid, strlen(can->mcu_controller_uid));
	info->mcu_uid[strlen(can->mcu_controller_uid)] = '\0';
	info->isotp_timeo = ISOTP_TIMEOUT_DEF;
}

static void *_can_fsm_thread(__attribute__((unused)) void *arg)
{
	struct can_frame *snd_frame = xmalloc(sizeof(struct can_frame));
	struct can_frame *rcv_frame = xmalloc(sizeof(struct can_frame));
	struct can_filter *filter = xmalloc(sizeof(struct can_filter));
	bool first_run = true;
	conn_can_raw_t conn;
	int r;

	_add_can_info();
	state_init();

try_again:

	if (state_is(TOFIX))
		state_event(FIXED);

	if (!_raw_init(&conn, snd_frame, filter))
		goto err;

	do {
		/*
		 * The raw frame must be sent until the HOST doesn't configure
		 * the IHB
		 */
		if (state_is(NOTIFY)) {
			if (_raw_frame_snd(&conn, snd_frame) < 0)
				break;

			if(_raw_rcv_set_filter(&conn, filter) < 0)
				break;
		}

		/*
		 * In IDLE state, the _raw_frame_snd is not need then set the
		 * filter only one time (the first time)
		 */
		if (first_run) {
			_raw_rcv_set_filter(&conn, filter);
			first_run = false;
		}

		/*
		 * rcv raw can socket:
		 *
		 * When conn_can_raw_recv goes in timeout it returns -ETIMEDOUT.
		 * https://github.com/RIOT-OS/RIOT/issues/13744
		 */
		while (((r = conn_can_raw_recv(&conn, rcv_frame, RCV_TIMEOUT))
			 == sizeof(struct can_frame))) {

			print_raw_frame(rcv_frame);

			if (rcv_frame->can_dlc == 8)
				_raw_frame_analize(rcv_frame);
		}

	} while (state_is(IDLE) || state_is(NOTIFY) || state_is(BACKUP));

	r = conn_can_raw_close(&conn);
	if (r < 0)
		printf("[!] error closing the CAN socket: err=%d\n", r);

err:
	free(snd_frame);
	free(rcv_frame);
	free(filter);

	if (state_is(TOFIX)) {
		first_run = true;
		goto try_again;
	}

	/* Switch to transmission */
	if (state_is(ACTIVE)) {
		xtimer_usleep(WAIT_100ms);
		_start_isotp_tx();
	}

	return NULL;
}

void ihb_can_module_info(struct ihb_can_ctx *can)
{
	printf("- CAN: struct ihb_can_ctx address=%p, size=%ubytes",
		(void *)can,
		sizeof(struct ihb_can_ctx));

	printf("\n\tCAN controller num=%d\n\tCAN driver name=%s\n\tMCU id=%s\n\tCAN frame id=%#x\n",
		can->can_perh_id,
		can->can_drv_name,
		can->mcu_controller_uid,
		can->can_frame_id);

	printf("- FSM state = %s\n", state_print());

	return;
}

int ihb_can_init(void *ctx, kernel_pid_t _data_source)
{
	struct ihb_ctx *IHB = ctx;
	uint8_t unique_id[CPUID_LEN];
	kernel_pid_t pid;
	char *b;

	can = xmalloc(sizeof(struct ihb_can_ctx));
	memset(can, 0, sizeof(struct ihb_can_ctx));

	/* Get the Unique identifier from the MCU */
	cpuid_get(unique_id);
	if(ENABLE_DEBUG) {
		printf("[#] Unique ID: 0x");
		for (uint8_t i = 0; i < CPUID_LEN; i++)
			printf("%02x", unique_id[i]);
		puts("");
	}

	b = data2str(unique_id, CPUID_LEN);
	strcpy(can->mcu_controller_uid, b);
	free(b);

	/* For STM target the least significant bit are first two.. */
	LSBytes[0] = unique_id[0];
	LSBytes[1] = unique_id[1];

	add_fix[4] = unique_id[0];
	add_fix[5] = unique_id[1];

	/*
	 * Generate an Unique CAN ID from the MCU's unique ID
	 * the fletcher8 hash function returns a not null value
	 */
	can->can_frame_id = fletcher8(unique_id, MAX_MCU_ID_LEN);
#if defined(IHB_FORCE_CAN_ID)
	can->can_frame_id = IHB_FORCE_CAN_ID;
#endif
	DEBUG("[*] CAN ID=%d\n", can->can_frame_id);

	if( _scan_for_controller(can) != 0) {
		/* this should never happened */
		puts("[!] cannot binding the can controller");
		free(can);
		return -1;
	}

	pid = thread_create(can_handler_node_stack,
			    sizeof(can_handler_node_stack),
			    THREAD_PRIORITY_MAIN - 2,
			    THREAD_CREATE_SLEEPING,
			    _can_fsm_thread, NULL,
			    "ihb can handler");

	if(pid < KERNEL_PID_UNDEF) {
		puts("[!] cannot create ihb can handler thread");
		free(can);
		return -1;
	}

	/* Save the can CAN context */
	IHB->can = can;

	/* Get the ihb_info context */
	info = &IHB->ihb_info;

	/* Get the PID which generates the data to sent */
	pid_of_data_source = _data_source;

	DEBUG("[*] IHB: init CAN subumodule success\n");

	thread_wakeup(pid);
	return pid;
}
