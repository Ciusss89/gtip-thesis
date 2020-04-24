/**
 * Author:	Giuseppe Tipaldi
 * Created:	2020
 **/
#define _GNU_SOURCE

#include <inttypes.h>
#include <string.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

/* RIOT APIs */
#include "can/conn/isotp.h"
#include "can/device.h"
#include "thread.h"
#include "msg.h"

#define ENABLE_DEBUG    (0)
#include "debug.h"

#include "ihb-tools/tools.h"

#define RECEIVE_THREAD_MSG_QUEUE_SIZE   (4)

#include "can.h"

static kernel_pid_t pid_send2host;
static struct ihb_can_perph *can = NULL;
bool sck_ready = false;

void ihb_isotp_send_chunks(void *in_data, size_t data_bs, size_t nmemb)
{
	if (!sck_ready || !in_data || data_bs == 0 || nmemb < 1)
		return;

	struct buffer_info *b = xmalloc(sizeof(struct buffer_info));
	b->length = data_bs * nmemb;
	msg_t msg;
	void *p;

	/*
	 * Pay attention to memory consuming...
	 */
	p = xcalloc(data_bs, nmemb);

	memcpy(p, in_data, b->length);

	DEBUG("[#] serialized, obj=%ubytes, data_bs=%dbytes nmemb=%ubytes\n",
	      b->length, data_bs, nmemb);

	b->data = p;

	msg.type = CAN_MSG_SEND_ISOTP;
	msg.content.ptr = b;
	msg_send (&msg, pid_send2host);
	free(b);
}

void *_thread_send2host(void *in)
{
	struct ihb_structs *IHB = (struct ihb_structs *)in;
	//struct isotp_fc_options *fc_opt;
	struct isotp_options isotp_opt;
	conn_can_isotp_t conn;
	msg_t msg, msg_queue[RECEIVE_THREAD_MSG_QUEUE_SIZE];
	int r;

	/* Saves the pointer of structs */
	can = IHB->can;

	/* setup the device layers message queue */
	msg_init_queue(msg_queue, RECEIVE_THREAD_MSG_QUEUE_SIZE);

	/* setup the socket */
	memset(&isotp_opt, 0, sizeof(isotp_opt));
	isotp_opt.tx_id = 0x700;
	isotp_opt.rx_id = 0x708;
	isotp_opt.txpad_content = 0xCE;

	pid_send2host = thread_getpid();

	while (1) {
		msg_receive(&msg);
		switch (msg.type) {
			case CAN_MSG_START_ISOTP:

				if(sck_ready)
					break;

				/* Start the IS0-TP socket */
				r = conn_can_isotp_create(&conn,
							  &isotp_opt,
							  can->id);
				if(r < 0) {
					printf("[!] cannot create the CAN socket: err=%d\n", r);
					break;
				} else if(r == 0) {

					r = conn_can_isotp_bind(&conn, NULL);
					if(r < 0) {
						printf("[!] cannot bind the CAN socket: err=%d\n", r);
						break;
					}

					sck_ready = true;
					DEBUG("[#] The IS0-TP socket has been stared\n");
				}

				break;
			case CAN_MSG_SEND_ISOTP:
				if(!sck_ready)
					break;

				struct buffer_info *b = msg.content.ptr;

				/*
				 * When we are sending a blocking message we
				 * adds overhead to communication because I
				 * wait for tx response.
				 *
				 * CAN_ISOTP_TX_DONT_WAIT make it not blocking.
				 */
				r = conn_can_isotp_send(&conn, b->data, b->length, 0);
				if(r < 0) {
					printf("[!] iso-tp send err=%d\n", r);
					if(r == -EIO)
						puts("[!] rcv socket not ready");
					buffer_clean(b);
					break;
				}

				if(r > 0 && (size_t)r != b->length)
					puts("[!] short send\n");

				buffer_clean(b);
				DEBUG("[#] The IS0-TP message has been sent\n");
				break;
			case CAN_MSG_CLOSE_ISOTP:

				if(!sck_ready)
					break;

				/* Close the IS0-TP socket */
				r = conn_can_isotp_close(&conn);
				if(r < 0)
					printf("[!] cannot close the CAN socket: err=%d\n", r);

				DEBUG("[#] The IS0-TP socket has been closed\n");
				sck_ready = false;
				break;
			default:
				puts("[!] received unknown message");
				break;
		}
	}

	return NULL;
}
