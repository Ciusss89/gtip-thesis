#ifndef IHB_H
#define IHB_H

#include <stdio.h>
#include <stdbool.h>

#define ATTR_UNUSED __attribute__((unused))

#define WAIT_2000ms	(2000LU * US_PER_MS)	/* delay of 2s */
#define WAIT_1000ms	(1000LU * US_PER_MS)	/* delay of 1s */
#define WAIT_500ms	(500LU * US_PER_MS)	/* delay of 500ms */
#define WAIT_100ms	(100LU * US_PER_MS)	/* delay of 100ms */
#define WAIT_20ms	(20LU * US_PER_MS)	/* delay of 020ms */
#define WAIT_10ms	(10LU * US_PER_MS)	/* delay of 010ms */
#define WAIT_5ms	(5LU * US_PER_MS)       /* delay of 005ms */

#define MAX_INFO_LENGHT (31)

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
 * struct ihb_ctx - it's a containter of ihb data, info and other stuff.
 *
 * @can: pointer of struc ihb_can_ctx
 * @sk_nodes: pointer of struct skin_node
 * @pid_ihbnetsim: pointer of skin_node_sim_thread pid
 * @pid_can_handler: can handler thread pid
 * @ihb_info: pointer of struc ihb_node_info
 */
struct ihb_ctx {
	void *can;
	void *sk_nodes;
	kernel_pid_t pid_skin_handler;
	kernel_pid_t pid_can_handler;
	struct ihb_node_info ihb_info;
};

#endif
