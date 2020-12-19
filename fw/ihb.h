#ifndef IHB_H
#define IHB_H

#include <stdio.h>
#include <stdbool.h>

/* Maximum length of INFOs argument */
#define MAX_INFO_LENGHT (31)

/* SK_N_S: Skin nodes for IHB */
#ifndef SK_N_S
#define SK_N_S (16)
#endif

/* Set the tx port for ISO TP transmissions */
#ifndef ISOTP_IHB_TX_PORT
#define ISOTP_IHB_TX_PORT 0x700
#endif

/* Set the rx port for ISO TP transmissions */
#ifndef ISOTP_IHB_RX_PORT
#define ISOTP_IHB_RX_PORT 0x708
#endif

/* SK_T_S: Skin Taxel sensors per skin node */
#define SK_T_S (12)

/* Each SPI bus can have almost 62 nodes */
#define SKIN_MAX_COUNT  (62 * 4)

/**
 * struct skin_node - represent the collected data which are incoming by skin
 *                    nodes. The struct skin_node is filled by pid_skin_handler
 *
 * @data: payload, the output of the tactile sensors
 * @address: node's address
 * @expired: true if a tactail node has passed way
 */
struct skin_node {
	uint16_t data[SK_T_S];
	uint8_t address;
	bool expired;
};

/**
 * struct ihb_node_info - contains the info of IHB node which are sent to HOST
 *
 * @mcu_uid: MCU's unique ID
 * @mcu_arch: MCU's architecture
 * @mcu_board: IHB board name
 * @riotos_ver: RIOT-OS release version
 * @ihb_fw_rev: IHB firmware release version
 * @skin_node_count: Number of skin nodes per IHB
 * @skin_node_taxel: Number of tactile sensors per skin node
 * @isotp_timeo: timeout for ISO TP transmissions
 */
struct ihb_node_info {
	char mcu_uid[MAX_INFO_LENGHT + 1];
	char mcu_arch[MAX_INFO_LENGHT + 1];
	char mcu_board[MAX_INFO_LENGHT + 1];

	char riotos_ver[MAX_INFO_LENGHT + 1];
	char ihb_fw_rev[MAX_INFO_LENGHT + 1];

	uint8_t skin_node_count;
	uint8_t skin_node_taxel;

	uint8_t isotp_timeo;
};

/**
 * struct ihb_ctx - IHB context struct
 *
 * @mcu_controller_uid: MCU's unique ID
 * @can_drv_name: the name of the can controller
 * @pid_skin_handler: pid of submodule skin driver
 * @pid_can_handler: pid of submodule can driver
 * @ihb_info: the info about the node which is sent to host
 * @can_frame_id: CAN frame identifier, it's obtainded by the MCU unique ID
 * @can_isotp_ready: if true the IHB can send the isotp chunks to host
 * @can_speed: CAN bitrates in bits
 * @can_perh_id: peripheral identifier of the MCU's CAN controller
 */
struct ihb_ctx {
	char mcu_controller_uid[MAX_INFO_LENGHT + 1];
	char can_drv_name[MAX_INFO_LENGHT + 1];
	int16_t pid_skin_handler;
	int16_t pid_can_handler;
	struct ihb_node_info ihb_info;
	uint8_t can_frame_id;
	uint8_t can_perh_id;
	uint32_t can_speed;
	bool can_isotp_ready;
};
#endif
