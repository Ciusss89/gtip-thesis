#ifndef IHB_H
#define IHB_H

#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */

#include <stdint.h>
#include <stdbool.h>

#include "uthash.h"

struct ihb_node {
	/* Data */
	uint16_t uid_LSBytes[256];
	uint8_t cnt;
	bool expired;
	bool best;
	int canID;

	/* makes this structure hashable */
	UT_hash_handle hh;
};

/* ihbs_cleanup() - clean and release the allocated memory
 *
 */
void ihbs_cleanup(void);

/*
 * ihb_init_socket_can() - open raw CAN communication with IHB
 *
 * @can_soc_fd: CAN raw socket
 * @d: CAN device
 * d: CAN device
 *
 * Returns 0 in case of success, num < 0 othrewise.
 */
int ihb_init_socket_can(int *can_soc_fd, const char *d);

/*
 * ihbs_wakeup() - wake up all IHBs nodes
 *
 * @s: socket can
 *
 * After the bootup the IHB node goes in IDLE stutus. It needs a wakeup message
 * to receive a role ACTIVE or BACKUP.
 *
 * In case of error returns a num < 0.
 */
int ihbs_wakeup(int s);

/*
 * ihb_setup() - send a role to all IHBs which has been discovered
 *
 * @s: socket can
 * @c_id_master: the CAN ID of IHB node which  will be set ACTIVE.
 * v: if true enable verbose
 *
 * Send a configuration message to assign a role to IHBs nodes
 *
 * In case of error returns a num < 0.
 */
int ihb_setup(int s, uint8_t c_id_master, bool v);

/*
 * ihb_discovery() - discovery online IHB nodes
 *
 * @fd: can raw socket
 * @best: CAN ID of the current best IHB node
 * @ihbs_cnt: amount of nodes which has been discovered
 * @ssigned_canID: input array used to track the CAN ID
 * @try_again: becomes true when a CAN ID collision is discovered
 * @verbose: if true enable verbose
 * @running: when false exit from loop
 *
 * ihb_discovery listens the raw frame which are incoming on CAN bus. If a frame
 * is valid the struct ihb_node will be filled with IHB's info like the CAN ID
 * frame and the two last bytes of MCU id.
 *
 * For each node which has been discovered:
 *   - increase the counter of @ihbs_cnt.
 *   - set as taken the CAN ID frame into @assigned_canID.
 *   - save lowest CAN ID frame into @best
 *   - save the two last bytes of MCU id into uid_LSBytes[0]
 *
 * When a collision happens:
 *   - set true the try_again.
 *   - save the two last bytes of MCU id into uid_LSBytes[i > 0]
 *
 * Returns 0 in case of success, num < 0 othrewise.
 */
int ihb_discovery(int fd, uint8_t *best, uint8_t *ihbs_cnt,
		  bool *assigned_canID, bool *try_again,
		  bool verbose, bool running);

/*
 * ihb_runtime_fix_collision() - fix IHB nodes which have an can ID collision.
 *
 * @fd: can raw socket
 * @assigned_canID: input array used to track the CAN ID
 *
 * Function is excuted only if try_again is true;
 * In this case all IHB nodes which are reported into uid_LSBytes[i > 0]
 * will receive a configuration message with the a new CAN ID.
 *
 * As last step the function clean realse memory alloceted to store struct ihb_node
 * and reset to false all assigned_canID
 *
 * Returns 0 in case of success, num < 0 othrewise.
 */
int ihb_runtime_fix_collision(int fd, bool *assigned_canID);

/*
 * ihb_blacklist_node() - set as expired an IHB node
 *
 * @ihb_expired: CAN ID of node which wants to set as expired
 * @v: if true enable verbose.
 *
 * Returns 0 in case of success, num < 0 othrewise.
 */
int ihb_blacklist_node(uint8_t ihb_expired, bool v);

/*
 * ihb_discovery_newone() - serach a new IHB node
 *
 * @master_id: where to store value of new best IHB node
 * @v: if true enable verbose
 *
 * It saves into @@master_id a new CAN ID nodes.
 *
 * Returns 0 in case of success, num < 0 othrewise.
 */
int ihb_discovery_newone(uint8_t *master_id, bool v);


/*
 * ihb_init_socket_can_isotp() - open isotp CAN communication with IHB
 *
 * @can_soc_fd: CAN isotp socket
 * @d: CAN device
 *
 * It starts an ISOTP can communication where:
 *   tx_id = 0x708
 *   rx_id = 0x700
 *
 * Returns 0 in case of success, num < 0 othrewise.
 */
int ihb_init_socket_can_isotp(int *can_soc_fd, const char *d);

/*
 * ihb_rcv_data() - receive the isotp data by ihb
 *
 * @can_soc_fd: socket isotp.
 * @verbose: when true prints the contents of received data.
 * @perf: when true prints the speed/timing info.
 * @running: when false exit from loop
 *
 *
 * The paramiters of CAN isotp link are set by the ihb_init_socket_can_isotp
 * The IHB sends as first package an info package which contains info about IHB
 * and the its skin modules.
 *
 * Verbose mode spams the console because prints the raw data of tactils sensors
 * The perf mode adds info about the realtime speed of the incoming data of IHB
 *
 * In case of error returns a num < 0.
 */
int ihb_rcv_data(int can_soc_fd, bool verbose, bool perf, bool running);

#endif
