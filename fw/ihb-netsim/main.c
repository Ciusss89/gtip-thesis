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
#include "random.h"

#define ENABLE_DEBUG    (1)
#include "debug.h"

#include "skin.h"

static struct skin_node sk[SK_N_S];

int netsim_sk_init(void)
{
	uint8_t i, j, b;

	printf("Skin node simulator: Nodes=%u Sensors per node%u\n", SK_N_S,
								     SK_T_S);

	/* Initialize */
	random_init(13);

	for(i = 0; i < SK_N_S; i++) {
		sk[i].address = i;

		for(j = 0; j < SK_T_S; j++) {
			random_bytes(&b, 1);
			sk[i].data[j] = b;
		}
	}

	if (ENABLE_DEBUG) {
		for(i = 0; i < SK_N_S; i++) {
			DEBUG("[0x%4x] Tactiles: ", sk[i].address);
			for(j = 0; j < SK_T_S; j++)
				DEBUG(" %d=%2x", j, sk[i].data[j]);
			puts(" ");
		}
	}
	return 0;
}
