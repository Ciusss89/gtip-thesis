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

#define ENABLE_DEBUG    (0)
#include "debug.h"

#include "ihb-tools/tools.h"
#include "skin.h"
#include "ihb.h"

#define SK_UPDATE_2000MS       (2000LU * US_PER_MS)    /* 2s */
#define SK_UPDATE_0100MS       (100LU * US_PER_MS)     /* 100ms */
#define SK_UPDATE_0005MS       (5LU * US_PER_MS)       /* 5ms */

struct skin_node *sk;

void *_skin_node_sim_thread(void *in)
{
	struct ihb_structs *IHB = (struct ihb_structs *)in;

	sk = xcalloc(SK_N_S, sizeof(struct skin_node));
	if(!sk)
		return NULL;

	*IHB->sk_nodes = sk;

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

		xtimer_usleep(SK_UPDATE_0005MS);
	}

	return NULL;
}
