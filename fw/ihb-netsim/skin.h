#ifndef CAN_NETSIM_H
#define CAN_NETSIM_H

#include <stdio.h>
#include <stdlib.h>

/* RIOT APIs */
#include "thread.h"

#define SK_THREAD_HELP	"skin simulator thread"

#define SK_UPDATE_2000MS	(2000LU * US_PER_MS)	/* 2s */
#define SK_UPDATE_0100MS	(100LU * US_PER_MS)	/* 100ms */
#define SK_UPDATE_0005MS	(5LU * US_PER_MS)	/* 5ms */

/* SK_N_S: Skin nodes for IHB */
#define SK_N_S (16u)

/* SK_T_S: Skin Tactile sensors per skin node */
#define SK_T_S (12u)

/**
 * @struct skin_node
 *
 * @data: payload, the output of the tactile sensors
 * @address: node address
 * @expired: each time that IHB acquires payload assume set it true, and
 *           it's set ony by skin node
 *
 * Struct size 114bytes.
 */
struct skin_node {
	uint8_t data[SK_T_S];
	uint16_t address;
	bool expired;
};

void *_skin_node_sim_thread(void *args);
#endif
