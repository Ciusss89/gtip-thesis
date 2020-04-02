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
#include "board.h"
#include "random.h"

#define ENABLE_DEBUG    (1)
#include "debug.h"

#define RECEIVE_THREAD_MSG_QUEUE_SIZE   (8)

#include "skin.h"

static struct skin_node sk[SK_N_S];

void *_skin_node_sim_thread(void *args)
{
	(void)(args);

	uint8_t i, j, b;
	msg_t msg, msg_queue[RECEIVE_THREAD_MSG_QUEUE_SIZE];

	/* setup the device layers message queue */
	msg_init_queue(msg_queue, RECEIVE_THREAD_MSG_QUEUE_SIZE);

	printf("Skin node simulator: Nodes=%u Sensors per node%u\n", SK_N_S,
								     SK_T_S);

	/* Initialize */
	random_init(13);

	while (1) {
		msg_receive(&msg);
		switch (msg.type) {
			case SK_MSG_UPDATE:

				for(i = 0; i < SK_N_S; i++) {
				sk[i].address = i;

					for(j = 0; j < SK_T_S; j++) {
						random_bytes(&b, 1);
						sk[i].data[j] = b;
					}
				}

				/* Printing to std breaks the timings */
				if (ENABLE_DEBUG) {
				for(i = 0; i < SK_N_S; i++) {
					DEBUG("[0x%04x] Tactiles: ", sk[i].address);
					for(j = 0; j < SK_T_S; j++)
						DEBUG(" %d=%02x", j, sk[i].data[j]);
					puts(" ");
					}
				}
				break;

			default:
				puts("thread: sk node sim: unknown message");
				break;
		}
	}

	return NULL;
}
