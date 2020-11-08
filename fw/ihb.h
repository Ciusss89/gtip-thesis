#ifndef IHB_H
#define IHB_H

#include <stdio.h>
#include <stdbool.h>

/* API's RIOT-OS */
#include "thread.h"

#define ATTR_UNUSED __attribute__((unused))
#define ATTR_NORET __attribute__((noreturn))

#define WAIT_2000ms	(2000LU * US_PER_MS)	/* delay of 2s */
#define WAIT_1000ms	(1000LU * US_PER_MS)	/* delay of 1s */
#define WAIT_500ms	(500LU * US_PER_MS)	/* delay of 500ms */
#define WAIT_100ms	(100LU * US_PER_MS)	/* delay of 100ms */
#define WAIT_20ms	(20LU * US_PER_MS)	/* delay of 020ms */
#define WAIT_10ms	(10LU * US_PER_MS)	/* delay of 010ms */
#define WAIT_5ms	(5LU * US_PER_MS)       /* delay of 005ms */

/* Maximum length for MCU id string */
#define MAX_MCU_ID_LEN (16u)
#if defined CPUID_LEN && MAX_MCU_ID_LEN <= CPUID_LEN
#error "CPUID_LEN > MAX_CPUID_LEN"
#endif

/* Maximum length of CAN driver name */
#define MAX_INFO_LENGHT (31)

/* Maximum length of CAN driver name */
#define MAX_NAME_LEN (16)

/* SK_N_S: Skin nodes for IHB */
#ifndef SK_N_S
#define SK_N_S (8u)
#endif

/* SK_T_S: Skin Tactile sensors per skin node */
#ifndef SK_T_S
#define SK_T_S (12u)
#endif

#define MAX_SK_NODES (256u)
#define MAX_SK_TACTAILS (16u)

/**
 * struct skin_node - represent the collected data which are incoming by skin
 *                    nodes. The struct skin_node is filled by pid_skin_handler
 *
 * @data: payload, the output of the tactile sensors
 * @address: node's address
 * @expired: true if a tactail node has passed way
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
 *
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
	char mcu_controller_uid[MAX_MCU_ID_LEN * 2 + 1];
	char can_drv_name[MAX_NAME_LEN + 1];
	kernel_pid_t pid_skin_handler;
	kernel_pid_t pid_can_handler;
	struct ihb_node_info ihb_info;
	uint8_t can_frame_id;
	uint8_t can_perh_id;
	uint32_t can_speed;
	bool can_isotp_ready;
};
#endif
