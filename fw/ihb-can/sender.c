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
#include "msg.h"

#define ENABLE_DEBUG    (1)
#include "debug.h"

#define RECEIVE_THREAD_MSG_QUEUE_SIZE   (4)

#include "can.h"

static struct ihb_can_perph *can = NULL;
#ifdef MODULE_IHBNETSIM
static struct skin_node *sk_nodes = NULL;
#endif

void *_thread_send2host(void *in)
{
	struct ihb_structs *IHB = (struct ihb_structs *)in;
	msg_t msg, msg_queue[RECEIVE_THREAD_MSG_QUEUE_SIZE];

	/* Saves the pointer of structs */
	sk_nodes = *IHB->sk_nodes;
#ifdef MODULE_IHBNETSIM
	can = *IHB->can;
#endif
	/* setup the device layers message queue */
	msg_init_queue(msg_queue, RECEIVE_THREAD_MSG_QUEUE_SIZE);

	while (1) {
		msg_receive(&msg);
		switch (msg.type) {
			case CAN_MSG_START_ISOTP:
				puts("------------WIP--------------");
				printf("TEST can-id=%d\n", can->frame_id);
#ifdef MODULE_IHBNETSIM
				if(ENABLE_DEBUG) {
					puts("------------struct skin_node--------------");
					for(uint8_t i = 0; i < SK_N_S; i++) {
						DEBUG("[0x%04x] Tactiles: ", sk_nodes[i].address);
						for(uint8_t j = 0; j < SK_T_S; j++)
							DEBUG(" %d=%02x", j, sk_nodes[i].data[j]);
						puts(" ");
					}
				}
#endif
				break;
			default:
				puts("[!] received unknown message");
				break;
		}

	}

	return NULL;
}
