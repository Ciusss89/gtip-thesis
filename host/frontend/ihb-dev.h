#ifndef IHB_DEV_H
#define IHB_DEV_H
/* Max lenght for info */
#define MAX_INFO_LENGHT (31)

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

/**
 * struct ihb_node_info - contains the info of IHB node which are sent to HOST
 * @mcu_uid: MCU's unique ID
 * @mcu_arch: MCU's architecture
 * @mcu_board: IHB board name
 * @riotos_ver: RIOT-OS release version
 * @ihb_fw_rev: IHB firmware release version
 * @skin_nodes: Number of skin nodes per IHB
 * @skin_tactails: Number of tactile sensors per skin node
 */
struct ihb_node_info {
	char mcu_uid[MAX_INFO_LENGHT + 1];
	char mcu_arch[MAX_INFO_LENGHT + 1];
	char mcu_board[MAX_INFO_LENGHT + 1];

	char riotos_ver[MAX_INFO_LENGHT + 1];
	char ihb_fw_rev[MAX_INFO_LENGHT + 1];

	uint8_t skin_nodes;
	uint8_t skin_tactails;
};
#endif
