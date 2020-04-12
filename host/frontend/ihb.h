#ifndef IHB_H
#define IHB_H

#include <stdint.h>
#include <stdbool.h>

#include "uthash.h"

static const uint8_t IHBMAGIC[8] = {0x5F, 0x49, 0x48, 0x42, 0x05, 0xF5, 0x56, 0x5F};

struct ihb_node {
	/* Data */
	bool best, expired;
	char *canP;
	int canID;

	/* makes this structure hashable */
	UT_hash_handle hh;
};

static struct ihb_node *ihbs = NULL;

bool running;

int ihb_setup(int s, uint8_t c_id_master, bool v);

/*
 *
 */
int ihb_init_socket_can_isotp(int *can_soc_fd, const char *d, int dest);

/*
 *
 */
int ihb_init_socket_can(int *can_soc_fd, const char *d);

/*
 *
 */
int ihb_discovery(int fd, bool v, uint8_t *wanna_be_master, uint8_t *ihb_nodes);

/*
 *
 */
int ihb_rcv_data(int can_soc_fd, void **ptr, bool v);

/*
 *
 */
int ihb_blacklist_node(uint8_t ihb_expired, bool v);


#endif
