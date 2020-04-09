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
#include "can/conn/isotp.h"
#include "can/device.h"
#include "thread.h"
#include "msg.h"

#define ENABLE_DEBUG    (1)
#include "debug.h"

#include "ihb-tools/tools.h"

#define RECEIVE_THREAD_MSG_QUEUE_SIZE   (4)

#include "can.h"

static struct ihb_can_perph *can = NULL;
#ifdef MODULE_IHBNETSIM
static struct skin_node *sk_nodes = NULL;
const size_t nmemb = sizeof(struct skin_node);
const size_t buff_l = SK_N_S * nmemb;
#endif

static int serialize(void **obj)
{
	uint8_t i;
	void *p;

	p = xcalloc(SK_N_S, nmemb);

	for(i = 0; i < SK_N_S; i++)
		memcpy(p + (i * nmemb), sk_nodes, nmemb);

	DEBUG("[#] serialized, obj=%ubytes, nmemb=%ubytes\n", buff_l, nmemb);

	*obj = p;

	return 0;
}

void *_thread_send2host(void *in)
{
	struct ihb_structs *IHB = (struct ihb_structs *)in;
	struct isotp_options isotp_opt;
	struct conn_can_isotp conn;
	msg_t msg, msg_queue[RECEIVE_THREAD_MSG_QUEUE_SIZE];
	bool sck_ready = false;
	void *buff = NULL;
	int r;

	/* Saves the pointer of structs */
	sk_nodes = *IHB->sk_nodes;
#ifdef MODULE_IHBNETSIM
	can = *IHB->can;
#endif
	/* setup the device layers message queue */
	msg_init_queue(msg_queue, RECEIVE_THREAD_MSG_QUEUE_SIZE);

	/* setup the socket  */
	isotp_opt.tx_id = can->id;
	isotp_opt.rx_id = 0x0;
	isotp_opt.txpad_content = 0xCE;

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

				r = serialize(&buff);
				if(r < 0) {
					puts("[!] cannot serialize the struct\n");
					break;
				}

				r = conn_can_isotp_send(&conn, buff, buff_l, 0);
				if(r < 0) {
					printf("[!] iso-tp send err=%d\n", r);
					free(buff);
					break;
				}

				if(r != buff_l)
					puts("[!] short send\n");

				free(buff);
				buff = NULL;

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
