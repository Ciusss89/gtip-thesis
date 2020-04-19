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
#include "xtimer.h"
#include "msg.h"

#define ENABLE_DEBUG    (0)
#include "debug.h"

/*
 * IF true the system sends data chunks at maximum speed through a blocking
 * syscall. The maximum speed of transmission depends on CAN subsystem and
 * ISOTP channel options.
 *
 * If false the system sends data through a non-blocking syscall, so the
 * xtimer_usleep must be higher then CAN's time.
 */
#define BLOCKING (1)

#include "ihb-tools/tools.h"
#include "skin.h"
#include "ihb.h"

struct skin_node *sk;
#ifdef MODULE_IHBCAN
static kernel_pid_t *pid_send2host;
#endif
void *_skin_node_sim_thread(void *in)
{
	struct ihb_structs *IHB = (struct ihb_structs *)in;
#ifdef MODULE_IHBCAN
	msg_t msg;
	pid_send2host = IHB->pid_send2host;
#endif
	sk = xcalloc(SK_N_S, sizeof(struct skin_node));

	IHB->sk_nodes = sk;

	uint8_t i, j, b;

	printf("[*] Skin node simulator: Nodes=%u Sensors per node=%u\n", SK_N_S,
								      	  SK_T_S);

	/* Initialize */
	random_init(13);

	while (1) {

		for(i = 0; i < SK_N_S; i++) {
			sk[i].address = i;

			for(j = 0; j < SK_T_S; j++) {
				random_bytes(&b, 1);
				sk[i].data[j] = b;
				}
		}

		/* Printing to stdout breaks the timings */
		if (ENABLE_DEBUG) {
			for(i = 0; i < SK_N_S; i++) {
				DEBUG("[0x%04x] Tactiles: ", sk[i].address);
				for(j = 0; j < SK_T_S; j++)
					DEBUG(" %d=%02x", j, sk[i].data[j]);
				puts(" ");
			}
		}
#ifdef MODULE_IHBCAN
		msg.type = CAN_MSG_SEND_ISOTP;

		if (BLOCKING) {
			msg_send (&msg, *pid_send2host);
			/* #TODO FIX ME */
			xtimer_usleep(WAIT_500ms);
		} else {
			msg_try_send (&msg, *pid_send2host);
			xtimer_usleep(WAIT_500ms);
		}
#endif
	}

	return NULL;
}
