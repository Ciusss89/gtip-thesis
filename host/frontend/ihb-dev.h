#ifndef IHB_DEV_H
#define IHB_DEV_H
/* Max lenght for info */
#define MAX_INFO_LENGHT (31)

#define MAX_SK_NODES (256u)
#define MAX_SK_TACTAILS (16u)

/* SK_N_S: Skin nodes for IHB */
#define SK_N_S (16u)

/* SK_T_S: Skin Tactile sensors per skin node */
#define SK_T_S (12u)

/* ASCII message "IHB-ID" : */
const uint8_t IHBMAGIC[6] = {0x49, 0x48, 0x42, 0x2D, 0x49, 0x44};

/* ASCII message "IHB-WKUP" : switch the node in notify state */
const char *msg_wkup = "4948422D574B5550";
/* ASCII message "IHB-MSTR" : switch the node in action state */
const char *msg_mstr = "4948422D4D535452";
/* ASCII message "IHB-BCKP" : switch the node in backup state */
const char *msg_bckp = "4948422D42434B50";
/* ASCII message "IHB-" : receive new address by ihbtool, runtime fix */
const char *msg_add_fix = "4948422D";

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
