#ifndef IHB_H
#define IHB_H

#include <stdio.h>
#include <stdbool.h>

#ifdef MODULE_IHBNETSIM
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
#define SK_THREAD_HELP	"ihb-skin net simulator"
#endif

#ifdef MODULE_IHBCAN
/* Maximum length of CAN name */
#define CAN_NAME_LEN (16 + 1)

#define MAX_CPUID_LEN (16)

/*
 * MCU can have one or more CAN controllers, by default I use the
 * CAN controlller 0
 */
#define CAN_C (0)

/**
 * struct skin_node - contains all data that are used by ihb-can driver
 *
 * @controller_uid: MCU's unique ID
 * @name: the name of the can controller
 * @id: identifier number of the MCU's can controller
 * @frame_id: CAN frame ID, it's obtainded by the MCU unique ID
 * @status_notify_node: if true the IHB node is announcing itself on CAN bus
 * @master: true when the IHB node is master, false otherwise.
 */
struct ihb_can_perph {
	char controller_uid[MAX_CPUID_LEN * 2 + 1];
	char name[CAN_NAME_LEN];
	uint8_t id;
	uint8_t frame_id;
	bool status_notify_node;
	bool master;
};

#define IHB_THREAD_HELP "ihb-can driver"
#endif

/**
 * struct ihb_structs - it's a containter of pointers
 *
 * The modules must be independent on one each other
 */
struct ihb_structs {
#ifdef MODULE_IHBCAN
	struct ihb_can_perph **can;
#endif
#ifdef MODULE_IHBNETSIM
	struct skin_node **sk_nodes;
#endif
};

#endif
