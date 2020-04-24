#ifndef IHB_H
#define IHB_H

#include <stdio.h>
#include <stdbool.h>

/* RIOT APIs */
#include "thread.h"

#define WAIT_2000ms	(2000LU * US_PER_MS)	/* delay of 2s */
#define WAIT_1000ms	(1000LU * US_PER_MS)	/* delay of 1s */
#define WAIT_500ms	(500LU * US_PER_MS)	/* delay of 500ms */
#define WAIT_100ms	(100LU * US_PER_MS)	/* delay of 100ms */
#define WAIT_20ms	(20LU * US_PER_MS)	/* delay of 020ms */
#define WAIT_10ms	(10LU * US_PER_MS)	/* delay of 010ms */
#define WAIT_5ms	(5LU * US_PER_MS)       /* delay of 005ms */

#ifdef MODULE_IHBNETSIM
#define MAX_SK_NODES (256u)
#define MAX_SK_TACTAILS (16u)

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
/*
 * IPC messages:
 */
#define CAN_MSG_START_ISOTP	0x400
#define CAN_MSG_SEND_ISOTP	0x401
#define CAN_MSG_CLOSE_ISOTP	0x402

/* Maximum length of CAN name */
#define CAN_NAME_LEN (16 + 1)

#define MAX_CPUID_LEN (16)

/*
 * MCU can have one or more CAN controllers, by default I use the
 * CAN controlller 0
 */
#define CAN_C (0)

/* timeout for ISO TP transmissions */
#define ISOTP_TIMEOUT_DEF (3u)

/**
 * struct skin_node - contains all data that are used by ihb-can driver
 *
 * @controller_uid: MCU's unique ID
 * @name: the name of the can controller
 * @id: identifier number of the MCU's can controller
 * @frame_id: CAN frame ID, it's obtainded by the MCU unique ID
 * @status_notify_node: if true the IHB node is announcing itself on CAN bus
 * @isotp_ready: true when the IHB can send the isotp chunks to host
 * @master: true when the IHB node is master, false otherwise.
 */
struct ihb_can_perph {
	char controller_uid[MAX_CPUID_LEN * 2 + 1];
	char name[CAN_NAME_LEN];
	uint8_t id;
	uint8_t frame_id;
	bool status_notify_node;
	bool isotp_ready;
	bool master;
};

#define IHB_THREAD_HELP "ihb-can driver"
#endif

#define MAX_INFO_LENGHT (31)
/**
 * struct ihb_structs - it's a containter of pointers
 *
 * The modules must be independent on one each other
 */
struct ihb_structs {
#ifdef MODULE_IHBCAN
	struct ihb_can_perph *can;
	kernel_pid_t *pid_notify_node;
	kernel_pid_t *pid_send2host;
#endif
#ifdef MODULE_IHBNETSIM
	struct skin_node *sk_nodes;
	kernel_pid_t *pid_ihbnetsim;
#endif
	struct ihb_node_info *ihb_info;
};

/**
 * struct ihb_node_info - contains the info of IHB node which are sent to HOST
 * @mcu_uid: MCU's unique ID
 * @mcu_arch: MCU's architecture
 * @mcu_board: IHB board name
 * @riotos_ver: RIOT-OS release version
 * @ihb_fw_rev: IHB firmware release version
 * @skin_nodes: Number of skin nodes per IHB
 * @skin_tactails: Number of tactile sensors per skin node
 * @isotp_timeo: timeout for ISO TP transmissions
 */
struct ihb_node_info {
	char mcu_uid[MAX_INFO_LENGHT + 1];
	char mcu_arch[MAX_INFO_LENGHT + 1];
	char mcu_board[MAX_INFO_LENGHT + 1];

	char riotos_ver[MAX_INFO_LENGHT + 1];
	char ihb_fw_rev[MAX_INFO_LENGHT + 1];

	uint8_t skin_nodes;
	uint8_t skin_tactails;

	uint8_t isotp_timeo;
};

#endif
