#ifndef CAN_NETSIM_H
#define CAN_NETSIM_H

#include <stdio.h>
#include <stdlib.h>

#define DLY_1000ms	(1000LU * US_PER_MS)	/* delay of 1 s */
#define DLY_100ms	(100LU * US_PER_MS)	/* delay of 1 s */

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

int netsim_sk_init(void);
#endif
