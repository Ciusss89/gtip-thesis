#ifndef SKIN_H
#define SKIN_H
/* SK_N_S: Skin nodes for IHB */
#define SK_N_S (16u)

/* SK_T_S: Skin Tactile sensors per skin node */
#define SK_T_S (12u)

/**
 * struct skin_node - represent the collected data that is incoming by skin nodes
 *
 * The Struct is filled by ihb-netsim driver
 *
 * @data: payload, the output of the tactile sensors
 * @address: node's address
 * @expired: Each time that the IHB acquires payload from SKIN node, that means
 *           that the node is still alive so expired is not set.
 *           Netsim set is always false.
 *
 * Struct size dipends by SK_T_S, if SK_T_S == 12, struct is 16bytes.
 */
struct skin_node {
	uint8_t data[SK_T_S];
	uint16_t address;
	bool expired;
};
#endif
