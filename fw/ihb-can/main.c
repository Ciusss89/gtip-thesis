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

#define WAIT_1000ms	(1000LU * US_PER_MS)	/* delay of 1 s */
#define WAIT_100ms	(100LU * US_PER_MS)	/* delay of 1 s */
#define RCV_TIMEOUT	(2000U * US_PER_MS)	/* socket rcv timeout */

static char notify_node_stack[THREAD_STACKSIZE_MEDIUM];
static kernel_pid_t pid_notify_node;

static char send2host_stack[THREAD_STACKSIZE_MEDIUM];
static kernel_pid_t pid_send2host;

struct ihb_can_perph *can;

static void _usage(void)
{
	puts("IHBCAN userspace");
	puts("\tihbcan canON     - turn on can controller");
	puts("\tihbcan canOFF    - turn off can controller");
	puts("\tihbcan notifyOFF - teardown pid_notify_node");
	puts("\tihbcan send2host - teardown notify, send isotp frame");
}

static int  _scan_for_controller(struct ihb_can_perph *device)
{
	const char *raw_can = raw_can_get_name_by_ifnum(CAN_C);

	if (raw_can && strlen(raw_can) < CAN_NAME_LEN) {
		device->id = CAN_C;
		strcpy(device->name, raw_can);
		DEBUG("[#] CAN controller=%d, name=%s\n", device->id, device->name);
		return 0;
	}

	return 1;
}

static uint8_t _power_up(uint8_t ifnum)
{
	uint8_t ret;

	if (ifnum >= CAN_DLL_NUMOF) {
		puts("[!] Invalid interface number");
		return 1;
	}

	ret = raw_can_power_up(ifnum);
	if (ret != 0) {
		printf("[!] Error when powering up: err=-%d\n", ret);
		return 1;
	}

	return 0;
}

static uint8_t _power_down(uint8_t ifnum)
{

	uint8_t ret = 0;
	if (ifnum >= CAN_DLL_NUMOF) {
		puts("[!] Invalid interface number");
		return 1;
	}

	ret = raw_can_power_down(ifnum);
	if (ret != 0) {
		printf("[!] Error when powering up: err=-%d\n", ret);
		return 1;
	}

	return 0;
}

static void *_thread_notify_node(void *arg)
{
	(void)arg;

	struct can_frame *snd_frame = xmalloc(sizeof(struct can_frame));
	struct can_frame *rcv_frame = xmalloc(sizeof(struct can_frame));
	struct can_filter *filter = xmalloc(sizeof(struct can_filter));
	conn_can_raw_t conn;
	msg_t msg;
	int r;

	can->status_notify_node = true;

	snd_frame->can_id = can->frame_id;
	snd_frame->can_dlc = 8;

	/*
	 * Send IHB's asci magic string "_IHB_V.3_"
	 *
	 * TODO! Pass the magic as configurable paramiter from the Makefile
	 */
	snd_frame->data[0] = 0x5F;
	snd_frame->data[1] = 0x49;
	snd_frame->data[2] = 0x48;
	snd_frame->data[3] = 0x42;
	snd_frame->data[4] = 0x05;
	snd_frame->data[5] = 0xF5;
	snd_frame->data[6] = 0x56;
	snd_frame->data[7] = 0x5F;

	r = conn_can_raw_create(&conn, NULL, 0, can->id, 0);
	if (r < 0) {
		printf("[!] cannot create the CAN socket: err=%d\n", r);
		return NULL;
	}

	/*
	 * Setup the filter for rcv socket
	 */
	filter->can_id = can->frame_id;
	filter->can_mask = CAN_SFF_MASK | CAN_RTR_FLAG;

	do {
		/*
		 * The raw frame must be sent until the HOST doesn't discovery
		 * me. When the HOST has assigned all roles to the IHBs nodes,
		 * the not masters nodes have to switch to listening mode.
		 */
		if(can->status_notify_node) {
			r = conn_can_raw_set_filter(&conn, NULL, 0);
			if (r < 0) {
				printf("[!] cannot remove filter from socket: err=%d", r);
				can->status_notify_node = false;
				break;
			}

			r = conn_can_raw_send(&conn, snd_frame, 0);
			if (r < 0) {
				printf("[!] error sending the CAN frame: err=%d\n", r);
				can->status_notify_node = false;
				break;
			}

			r = conn_can_raw_set_filter(&conn, filter, 1);
			if (r < 0) {
				printf("[!] cannot add the filter to the socket: err=%d", r);
				can->status_notify_node = false;
				break;
			}

			xtimer_usleep(WAIT_100ms);
		}

		/*
		 * Issue:
		 * I would make true the next while only if the RTR bit is set,
		 * but currently it enters also if the RTR is not set.
		 */
		while (((r = conn_can_raw_recv(&conn, rcv_frame, RCV_TIMEOUT))
			 == sizeof(struct can_frame))) {

			if(rcv_frame->can_dlc == 7) {
				can->status_notify_node = false;
				can->master = true;

				msg.type = CAN_MSG_START_ISOTP;
				msg_try_send(&msg, pid_send2host);

				puts("[*] This node is MASTER");

			} else if (rcv_frame->can_dlc == 3) {
				can->status_notify_node = false;
				can->master = false;

				puts("[*] This node is SLAVE");
			} else {
				/*
				 * This never should happen if:
				 *  - the RTR bit is set
				 *  - IHB are connected only to the HOST.
				 */
				if(ENABLE_DEBUG) {
					printf("[#] ihbcan: ID=%02lx  DLC=[%x] DATA=",
							rcv_frame->can_id,
							rcv_frame->can_dlc);

					for (int i = 0; i < rcv_frame->can_dlc; i++)
					printf(" %02X", rcv_frame->data[i]);
					puts("");
				}
				puts("BUG: I received an unexpected frame, it has been ignored.");
			}
		}

	/*
	 * Loop until
	 *  - no errors on the CAN APIs
	 *  - node have to stay in listend mode
	 */
	} while (r >= 0 && (!can->master || can->status_notify_node));

	r = conn_can_raw_close(&conn);
	if (r < 0)
		printf("[!] error closing the CAN socket: err=%d\n", r);

	free(snd_frame);
	free(rcv_frame);

	if(r >= 0)
		puts("[*] Notify thread has ended successfully");

	return NULL;
}

int _ihb_can_handler(int argc, char **argv)
{
	msg_t msg;

	if (argc < 2) {
		_usage();
		return 1;
	} else if (strncmp(argv[1], "canON", 6) == 0) {
		return _power_up(can->id);
	} else if (strncmp(argv[1], "canOFF", 7) == 0) {
		return _power_down(can->id);
	} else if (strncmp(argv[1], "notifyOFF", 10) == 0) {
		can->status_notify_node = false;
		printf("[*] ihb: terminate %d", pid_notify_node);
	} else if (strncmp(argv[1], "send2host", 10) == 0) {
		if(can->status_notify_node) {
			can->status_notify_node = false;
			msg.type = CAN_MSG_START_ISOTP;
			msg_try_send(&msg, pid_send2host);
			puts("[*] ihb: teardown notify, start the isotp socket");
		}
		msg.type = CAN_MSG_SEND_ISOTP;
		msg_try_send(&msg, pid_send2host);
		puts("[*] ihb: send istop...");
	}  else {
		printf("unknown command: %s\n", argv[1]);
		return 1;
	}

	return 0;
}

int _can_init(struct ihb_structs *IHB)
{
	uint8_t unique_id[CPUID_LEN];
	uint8_t r = 1;
	char *b;

	can = xmalloc(sizeof(struct ihb_can_perph));

	IHB->pid_notify_node = &pid_notify_node;
	IHB->pid_send2host = &pid_send2host;
	IHB->can = can;

	if(CAN_DLL_NUMOF == 0)
		puts("[!] no CAN controller avaible");
	else
		DEBUG("[#] MCU has %d can controller\n", CAN_DLL_NUMOF);

	if(CPUID_LEN > MAX_CPUID_LEN) {
		puts("[!] CPUID_LEN > MAX_CPUID_LEN");
		return 1;
	}

	can->master = false;

	/* Get the Unique identifier from the MCU */
	cpuid_get(unique_id);
	if(ENABLE_DEBUG) {
		printf("[#] Unique ID: 0x");
		for (uint8_t i = 0; i < CPUID_LEN; i++)
			printf("%02x", unique_id[i]);
		puts("");
	}

	b = xmalloc(CPUID_LEN * 2 + 1);
	b = data2str(unique_id, CPUID_LEN);
	
	/* Save the MCU unique ID */
	strcpy(can->controller_uid, b);
	free(b);

	/*
	 * Generate an Unique CAN ID from the MCU's unique ID
	 * the fletcher8 hash function returns a not null value
	 */
	can->frame_id = fletcher8(unique_id, CPUID_LEN);
	DEBUG("[*] CAN ID=%d\n", can->frame_id);

	r = _scan_for_controller(can);
	if(r != 0) {
		/* this should never happened */
		puts("[!] cannot binding the can controller");
		return 1;
	}

	pid_send2host = thread_create(send2host_stack,
				      sizeof(send2host_stack),
				      THREAD_PRIORITY_MAIN - 2,
				      THREAD_CREATE_WOUT_YIELD,
				      _thread_send2host, (void *)IHB,
				      "ihb send to host");
	if(pid_send2host < KERNEL_PID_UNDEF) {
		puts("[!] cannot create thread send to host");
		return 1;
	}

	pid_notify_node = thread_create(notify_node_stack,
					sizeof(notify_node_stack),
					THREAD_PRIORITY_MAIN - 1,
					THREAD_CREATE_WOUT_YIELD,
					_thread_notify_node, NULL,
					"ihb notify node");
	if(pid_notify_node < KERNEL_PID_UNDEF) {
		puts("[!] cannot create thread notify node");
		return 1;
	}

	/* IDEA:
	 *
	 * 1) NODO-iesimo: Power-ON tutti hanno un CAN-ID (1-255) derivato dal CPUID != 0
	 * 2) NODO-iesimo: Power-ON, INIT: Ogni nodeo manda un frame di IMALIVE
	 * 3) HOST: dump di tutti i frame con CAN-ID compreso tra 1 e 255
	 * 3) HOST: master quello che tra ha CAN-ID minore.
	 * 4) HOST: Assegna al can-id master, CAN-iD 0
	 * 5) HOST: avvisa gli altri nodi che non sono master.
	 * 5) HODO-iesimo: non master dump per can-id 0. Ok se traffico
	 * 6) HOST: avvia la comunicazione con il mater.
	 *
	 * */

	puts("[*] IHB: init of the CAN subumodule success");
	return 0;
}
