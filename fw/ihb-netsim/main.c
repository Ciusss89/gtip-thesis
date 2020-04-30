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
#include "thread.h"
#include "xtimer.h"

#define ENABLE_DEBUG    (0)
#include "debug.h"

#include "ihb-tools/tools.h"
#include "skin.h"
#include "ihb.h"

#ifdef MODULE_IHBCAN
#include "ihb-can/can.h"
#endif

const size_t data_bs = sizeof(struct skin_node);
struct skin_node *sk;

void *_skin_node_sim_thread(void *in)
{
	struct ihb_structs *IHB = (struct ihb_structs *)in;
	struct ihb_node_info *info = IHB->ihb_info;
	sk = xcalloc(SK_N_S, data_bs);

	IHB->sk_nodes = sk;

	info->skin_nodes = SK_N_S;
	info->skin_tactails = SK_T_S;

	uint8_t i, j, b;

	printf("[*] Skin node simulator: Nodes=%u Sensors per node=%u\n", SK_N_S,
								      	  SK_T_S);

	/* Initialize */
	random_init(13);

	/*
	 * PAY ATTENTION:
	 * If you run this module without MODULE IHBCAN or extra delay,
	 * this loop becomes a CPU killer. It's like
	 * while(1) {
	 * }
	 */
	while (1) {

		/* Fill the struct ihb_structs with fake data */
		for(i = 0; i < SK_N_S; i++) {

			/* Address... */
			sk[i].address = i;

			/* Tactails... */
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
		if (IHB->can->isotp_ready)
			ihb_isotp_send_chunks(sk, data_bs, SK_N_S);
		else
			thread_sleep();
#endif

		/*
		 * You can add a delay if you needs it
		 * xtimer_usleep(WAIT_20ms);
		 */
	}

	return NULL;
}
