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

static struct ihb_node *ihbs = NULL;

bool running;

/*
 * @ihbs_wakeup: - wake up all IHBs nodes
 *
 * @s: socket can
 *
 * In case of error returns a num < 0.
 */
int ihbs_wakeup(int s);

int ihb_setup(int s, uint8_t c_id_master, bool v);

/*
 *
 */
int ihb_init_socket_can_isotp(int *can_soc_fd, const char *d);

/*
 *
 */
int ihb_init_socket_can(int *can_soc_fd, const char *d);

/*
 * @ihb_discovery: - discovery online IHB nodes
 *
 * @fd: can raw socket
 * @wanna_be_master: CAN ID of the current best IHB node
 * @ihb_nodes: amount of all IHB nodes which has been discovered
 * @array: input array used to track the CAN ID
 * @v: if true enable verbose
 *
 * ihb_discovery listens the raw frame which are incoming on CAN bus. If a frame
 * is valid the struct ihb_node will be filled with IHB's info like the CAN ID
 * frame and the two last bytes of MCU id.
 *
 * For each node which has been discoverd:
 *   - increse the counter of @ihb_nodes.
 *   - sign as taken the CAN ID frame into @array
 *   - save lowest CAN ID frame into @wanna_be_master
 *
 * Returns 0 in case of success, num < 0 othrewise.
 */
int ihb_discovery(int fd, uint8_t *wanna_be_master, uint8_t *ihb_nodes, uint16_t **array, bool v);

/*
 * @ihb_rcv_data: - receive the isotp data by ihb
 *
 * @can_soc_fd: socket isotp.
 * @ptr: not used yet, pass NULL.
 * @verbose: when true prints the contents of received data.
 * @perf: when true prints the speed/timing info.
 *
 * In case of error returns a num < 0.
 */
int ihb_rcv_data(int can_soc_fd, void **ptr, bool verbose, bool perf);

/*
 *
 */
int ihb_blacklist_node(uint8_t ihb_expired, bool v);

/*
 *
 */
int ihb_discovery_newone(uint8_t *master_id, bool v);
#endif
