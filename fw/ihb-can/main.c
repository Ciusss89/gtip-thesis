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
#include <errno.h>

/* RIOT APIs */
#include "periph/cpuid.h"
#include "thread.h"
#include "xtimer.h"
#include "board.h"
#include "msg.h"
#include "can/conn/raw.h"
#include "can/device.h"
#include "can/can.h"

#define ENABLE_DEBUG 0
#include "debug.h"

#include "ihb-tools/tools.h"
#include "message.h"
#include "can.h"
#include "ihb.h"

#define CAN_THREAD_HELP "ihb-can thread"
#define RCV_TIMEOUT (2000U * US_PER_MS)	/* socket raw CAN rcv timeout */

#if CAN_SPEED == 0
#define SPEED 1000000 /*1MiB*/
#elif CAN_SPEED == 1
#define SPEED 750000 /* 750KiB */
#elif CAN_SPEED == 2
#define SPEED 500000 /* 500KiB */
#elif CAN_SPEED == 3
#define SPEED 250000 /* 250KiB*/
#else
#error "Bad speed paramiter"
#endif

static char can_handler_node_stack[THREAD_STACKSIZE_MEDIUM];
static struct ihb_ctx *IHB = NULL;
static uint8_t LSBytes[2];

static void _start_isotp_tx(void)
{
	/* Open the iso-tp socket */
	ihb_isotp_init(IHB->can_perh_id,
		       ISOTP_TIMEOUT_DEF,
		       &(IHB->can_isotp_ready));

	/* Validate the first chunk */
	if (ihb_isotp_send_validate(&IHB->ihb_info, sizeof(struct ihb_node_info)) != 0) {
		puts("[!] ihb: isotp: failure on chunk validation");
		goto err;
	}

	/* Send the IHB's infos as first chunk */
	if (ihb_isotp_send_chunks(&IHB->ihb_info, sizeof(struct ihb_node_info)) > 0) {
		/* Set connection ready */
		IHB->can_isotp_ready = true;

		/* Wake up the thread */
		if (thread_wakeup(IHB->pid_skin_handler) != 1) {
			puts("[!] ihb: cannot start the skin thread");
			goto err;
		}
		puts("[*] ihb: node is sending data");

	} else {
		puts("[!] ihb: cannot send the node info");
		goto err;
	}

	return;

err:
	IHB->can_isotp_ready = false;
	ihb_isotp_close();
	state_event(FAIL);
	puts("[!] ihb: failure");
	return;
}

static bool _raw_init(conn_can_raw_t *socket,
		      struct can_frame *frame,
		      struct can_filter *filter)
{
	int r;

	r = conn_can_raw_create(socket, NULL, 0, IHB->can_perh_id, 0);
	if (r < 0) {
		printf("[!] cannot create the CAN socket: err=%d\n", r);
		return false;
	}

	/* Configure the frame to be sent to host */
	frame->can_id = IHB->can_frame_id;
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
		 * Fix runtime the CAN identifier, last byte contains the new
		 * frame identifier which must be used by the node
		 */
		if (memcmp(&add_fix, frame->data, 7) == 0) {
			printf("[*] IHB CAN id received, NEW=%#x, OLD=%#x\n",
					frame->data[7],
					IHB->can_frame_id);
			if (frame->data[4] == LSBytes[0] &&
			    frame->data[5] == LSBytes[1])
				IHB->can_frame_id = frame->data[7];
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
		filter->can_id = IHB->can_frame_id;

	/* Set hw filer to recevive only the messages with filter->can_id */
	r = conn_can_raw_set_filter(socket, filter, 1);
	if (r < 0) {
		printf("[!] cannot add the filter to the socket: err=%d\n", r);
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
		printf("[!] cannot remove filter from socket: err=%d\n", r);
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
	strncpy(IHB->ihb_info.mcu_uid, IHB->mcu_controller_uid, strlen(IHB->mcu_controller_uid));
	IHB->ihb_info.mcu_uid[strlen(IHB->mcu_controller_uid)] = '\0';
	IHB->ihb_info.isotp_timeo = ISOTP_TIMEOUT_DEF;
}

static void *_can_fsm_thread(ATTR_UNUSED void *arg)
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

		/* there is no need to excute_raw_frame_snd on the first run */
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

void ihb_can_module_info(void)
{
	char *ptr = NULL;

	ptr = state_print();

	printf("CAN controller num=%d\nCAN driver name=%s\nMCU id=%s\nCAN bitrate=%"PRIu32"\nCAN frame id=%#x\nFSM state = %s\n",
		IHB->can_perh_id,
		IHB->can_drv_name,
		IHB->mcu_controller_uid,
		IHB->can_speed,
		IHB->can_frame_id,
		ptr);

	free(ptr);
}

static int  _controller_probe(void)
{
	const char *raw_can = raw_can_get_name_by_ifnum(CAN_C);
	int ret;

	if (!raw_can || strlen(raw_can) > MAX_NAME_LEN)
		goto err;

	/*
	 * Set the given bitrate/sample_point to the given controller
	 *  - Maxinum controller speed is 1MiB.
	 *  - The sample point is the location(in percent value) inside each bit period
	 *    where the CAN controller looks at the state of the bus and determines if
	 *    it is a logic zero (dominant) or logic one (recessive).
	 *    The CAN-in-Automation user’s group makes recommendations of where the
	 *    sample point should be for various bit rates: 87.5% the most common.
	 */
	ret = raw_can_set_bitrate(CAN_C, SPEED, 875);
	if (ret != 0)
		goto err;

	/* Acquire controller info */
	IHB->can_perh_id = CAN_C;
	strcpy(IHB->can_drv_name, raw_can);
	IHB->can_speed = SPEED;

	DEBUG("[#] CAN controller=%d, name=%s, bitrate=%u\n",
	      IHB->can_perh_id, IHB->can_drv_name, SPEED);
	return 0;

err:
	return -1;
}

int ihb_init_can(struct ihb_ctx *ihb_ctx)
{
	uint8_t unique_id[CPUID_LEN];
	IHB = ihb_ctx;
	char *b;

	/* Get the Unique identifier from the MCU */
	cpuid_get(unique_id);
	if(ENABLE_DEBUG) {
		printf("[#] Unique ID: 0x");
		for (uint8_t i = 0; i < CPUID_LEN; i++)
			printf("%02x", unique_id[i]);
		puts("");
	}

	b = data2str(unique_id, CPUID_LEN);
	strcpy(IHB->mcu_controller_uid, b);
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
	ihb_ctx->can_frame_id = fletcher8(unique_id, MAX_MCU_ID_LEN);
#if defined(IHB_FORCE_CAN_ID)
	ihb_ctx->can_frame_id = IHB_FORCE_CAN_ID;
#endif
	DEBUG("[*] CAN ID=%d\n", ihb_ctx->can_frame_id);

	if(_controller_probe() != 0) {
		/* this should never happened */
		puts("[!] cannot bind the can controller");
		return -1;
	}
	
	return  thread_create(can_handler_node_stack,
			      sizeof(can_handler_node_stack),
			      THREAD_PRIORITY_MAIN - 2,
			      THREAD_CREATE_SLEEPING,
			      _can_fsm_thread, NULL,
			      CAN_THREAD_HELP);
}
