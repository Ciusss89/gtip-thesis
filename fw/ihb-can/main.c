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
#include "shell.h"
#include "board.h"
#include "thread.h"
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

/* ASCII message "" */
static const unsigned char mstr[] = {0x49, 0x48, 0x42, 0x2D, 0x42, 0x55, 0x53, 0x59, 0};
/* ASCII message "" */
static const unsigned char bckp[] = {0x49, 0x48, 0x42, 0x2D, 0x49, 0x44, 0x4C, 0x45, 0};
/* ASCII message "_IHB..V_" */
static const unsigned char mgc[] = {0x5F, 0x49, 0x48, 0x42, 0x05, 0xF5, 0x56, 0x5F, 0};

static char can_handler_node_stack[THREAD_STACKSIZE_MEDIUM];

/*
 * PID of the process that generate the data which have to sent by isotp
 * transmission.
 */
static kernel_pid_t pid_of_data_source;

static struct ihb_node_info *info = NULL;
static struct ihb_can_ctx *can = NULL;

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

	/*  Configure the frame which has to be send */
	frame->can_id = can->can_frame_id;
	frame->can_dlc = 8;
	memcpy(frame->data, &mgc, 8);
	
	/* Setup the filter for rcv socket */
	filter->can_id = can->can_frame_id;
	filter->can_mask = CAN_SFF_MASK | CAN_RTR_FLAG;

	return true;
}


static bool _raw_frame_snd(conn_can_raw_t *socket,
			   struct can_frame *frame,
			   struct can_filter *filter)
{
	int r;

	/* Remove hw filter */
	r = conn_can_raw_set_filter(socket, NULL, 0);
	if (r < 0) {
		printf("[!] cannot remove filter from socket: err=%d", r);
		return false;
	}

	/* Send */
	r = conn_can_raw_send(socket, frame, 0);
	if (r < 0) {
		printf("[!] error sending the CAN frame: err=%d\n", r);
		return false;
	}

	/* Set hw filer */
	r = conn_can_raw_set_filter(socket, filter, 1);
	if (r < 0) {
		printf("[!] cannot add the filter to the socket: err=%d", r);
		return false;
	}

	xtimer_usleep(WAIT_100ms);

	return true;
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
	conn_can_raw_t conn;
	int r;

	if (_raw_init(&conn, snd_frame, filter))
		can->status_notify_node = true;
	else
		goto err;

	_add_can_info();

	do {

		/*
		 * The raw frame must be sent until the HOST doesn't discovery
		 * me. When the HOST has assigned all roles to the IHBs nodes,
		 * the not masters nodes have to switch to listening mode.
		 */
		if(can->status_notify_node) {
			if(!_raw_frame_snd(&conn, snd_frame, filter)) {
				can->status_notify_node = false;
				break;
			}
		}

		/*
		 * rcv raw can socket:
		 *
		 * When conn_can_raw_recv goes in timeout it returns -ETIMEDOUT.
		 * https://github.com/RIOT-OS/RIOT/issues/13744
		 */
		while (((r = conn_can_raw_recv(&conn, rcv_frame, RCV_TIMEOUT))
			 == sizeof(struct can_frame))) {

			if(ENABLE_DEBUG) {
				printf("[#] ihbcan: ID=%"PRIu32"  DLC=[%u] DATA=",
						rcv_frame->can_id,
						rcv_frame->can_dlc);
				for (int i = 0; i < rcv_frame->can_dlc; i++)
					printf(" %#x", rcv_frame->data[i]);
				puts("");
			}

			/* The message should be 8 byte lenght */
			if(rcv_frame->can_dlc == 8) {

				/* Is master ? */
				r = memcmp(&mstr, rcv_frame->data,
					   rcv_frame->can_dlc);
				if (r == 0) {
					can->status_notify_node = false;
					can->master = true;
					puts("[*] This node is MASTER");
					break;
				}

				/* Is slave ? */
				r = memcmp(&bckp, rcv_frame->data,
					   rcv_frame->can_dlc);
				if (r == 0) {
					can->status_notify_node = false;
					can->master = false;
					puts("[*] This node is a BACKUP node");
					break;
				}
			}
		}

	/*
	 * Loop condition
	 *  - raw send/rcv until no errors on the CAN APIs && status_notify_node
	 *    is true
	 *  - olny raw rcv until no errors on the CAN APIs && status_notify_node
	 *    if false
	 */
	} while (!can->master || can->status_notify_node);

	r = conn_can_raw_close(&conn);
	if (r < 0)
		printf("[!] error closing the CAN socket: err=%d\n", r);

err:
	free(snd_frame);
	free(rcv_frame);
	free(filter);

	/* Switch to transmission */
	if(can->master) {
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

	/*
	 * Generate an Unique CAN ID from the MCU's unique ID
	 * the fletcher8 hash function returns a not null value
	 */
	can->can_frame_id = fletcher8(unique_id, MAX_MCU_ID_LEN);
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
	info = IHB->ihb_info;

	/* Get the PID which generates the data to sent */
	pid_of_data_source = _data_source;

	DEBUG("[*] IHB: init CAN subumodule success");

	thread_wakeup(pid);
	return pid;
}
