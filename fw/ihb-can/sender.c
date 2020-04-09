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
				DEBUG("[#] The IS0-TP socket has been stared\n");
				break;
			case CAN_MSG_SEND_ISOTP:
				DEBUG("[#] The IS0-TP message has been sent\n");
				break;
			case CAN_MSG_CLOSE_ISOTP:
				DEBUG("[#] The IS0-TP socket has been closed\n");
				break;
			default:
				puts("[!] received unknown message");
				break;
		}
	}

	return NULL;
}
