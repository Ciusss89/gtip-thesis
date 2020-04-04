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

#define RECEIVE_THREAD_MSG_QUEUE_SIZE   (4)

#include "ihbcan.h"

void *_thread_send2host(void *device)
{
	struct ihb_can_perph *d = (struct ihb_can_perph *)device;

	msg_t msg, msg_queue[RECEIVE_THREAD_MSG_QUEUE_SIZE];

	/* setup the device layers message queue */
	msg_init_queue(msg_queue, RECEIVE_THREAD_MSG_QUEUE_SIZE);

	while (1) {
		msg_receive(&msg);
		switch (msg.type) {
			case CAN_MSG_START_ISOTP:
				puts("------------WIP--------------");
				printf("ALLA IS GOOD %d\n", d->frame_id );
				break;
			default:
				puts("[!] received unknown message");
				break;
		}

	}

	return NULL;
}
