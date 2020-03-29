#define _GNU_SOURCE
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>

#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#include "ihb.h"
#include "lib.h"

static struct ihb_node *find_canID(int can_id) {
	struct ihb_node *s;

	HASH_FIND_INT(ihbs, &can_id, s);  /* s: output pointer */
	return s;
}

int ihb_setup(int s, uint8_t c_id_master, bool v)
{
	const uint8_t MASTER = 8, IDLE = 4;
	struct canfd_frame frame;
	struct ihb_node *ihb;

	int r = -1, required_mtu = -1;
	char *cmd;

	for(ihb = ihbs; ihb != NULL; ihb = (struct ihb_node *)(ihb->hh.next)) {
		fprintf(stdout, "configuring IHB node=%02x\n", ihb->canID);

		r = asprintf(&cmd, "%03x#R", ihb->canID);
		if (r < 0) {
			fprintf(stderr, "ihb_setup: asprintf fails");
			break;
		}

		if(ihb->canID == c_id_master & !ihb->expired) {
			/*
			 * Parse CAN frame
			 *
			 * Transfers a valid ASCII string describing a CAN frame
			 * into struct canfd_frame
			 */
			required_mtu = parse_canframe(cmd, &frame);
			frame.len = MASTER;

			/* send frame */
			if (write(s, &frame, required_mtu) != required_mtu) {
				r = -1;
				break;
			}

			ihb->best = true;

			free(cmd);
		} else {
			/*
			 * Parse CAN frame
			 *
			 * Transfers a valid ASCII string describing a CAN frame
			 * into struct canfd_frame
			 */
			required_mtu = parse_canframe(cmd, &frame);
			frame.len = IDLE;

			/* send frame */
			if (write(s, &frame, required_mtu) != required_mtu) {
				r = -1;
				break;
			}

			ihb->best = false;

			free(cmd);
		}
	}

	return r;
}


int ihb_discovery(int fd, bool v, uint8_t *wanna_be_master, uint8_t *ihb_nodes)
{
	struct timeval timeout_config = { 10, 0 };
	struct can_frame frame_rd;
	struct ihb_node *ihb;
	ssize_t recv_bytes = 0;
	bool discovery = true;
	fd_set rdfs;
	int r, i;


	while (discovery) {
		FD_ZERO(&rdfs);
		FD_SET(fd, &rdfs);

		r = select(fd + 1, &rdfs, NULL, NULL, &timeout_config);

		if (r < 0) {
			fprintf(stderr, "can_soc_fd not ready: %s\n", strerror(errno));
			discovery = false;
			continue;
		}

		if (r == 0) {
			fprintf(stdout, "discovery phase has been elapsed\n");
			discovery = false;
			continue;
		}

		if (FD_ISSET(fd, &rdfs)) {

			recv_bytes = read(fd, &frame_rd, sizeof(struct can_frame));
			if (!recv_bytes) {
				fprintf(stderr, "read failure: %s\n", strerror(errno));
				continue;
			}

			if(v) {
				fprintf(stdout, "id = %02x  dlc = [%d], data = ",
						frame_rd.can_id,
						frame_rd.can_dlc);
				for (i = 0; i < frame_rd.can_dlc; i++)
					printf("%02x", frame_rd.data[i]);
				printf("\n");
			}

			if (memcmp(frame_rd.data, IHBMAGIC, frame_rd.can_dlc) == 0) {

				ihb = find_canID(frame_rd.can_id);
				if (!ihb) {

					ihb = (struct ihb_node *)malloc(sizeof(struct ihb_node));
					if (ihb == NULL) {
						fprintf(stderr, "malloc failure:\n");
						discovery = false;
						return -1;
					}

					ihb->expired = false;
					ihb->canID = frame_rd.can_id;
					ihb->canP = NULL; /* not used for now */

					HASH_ADD_INT(ihbs, canID, ihb);

					fprintf(stdout, "ihb node %02x has been added\n",
							ihb->canID);

					*ihb_nodes = *ihb_nodes + 1;
				}

				if (frame_rd.can_id <= *wanna_be_master)
					*wanna_be_master = frame_rd.can_id;

			}

			
		} else {
			discovery = false;
			continue;
		}
	}

	return 0;
}

int ihb_init_socket_can(int *can_soc_fd, const char *d)
{
	struct sockaddr_can addr;
	struct ifreq ifr;

	int r;

	*can_soc_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if (*can_soc_fd < 0) {
		fprintf(stderr, "socket open failure! %s\n", strerror(errno));
		return -errno;
	}

	addr.can_family = AF_CAN;
	memset(&ifr.ifr_name, 0, sizeof(ifr.ifr_name));
	strncpy(ifr.ifr_name, d, sizeof(d) + 1);

	r = ioctl(*can_soc_fd, SIOCGIFINDEX, &ifr);
	if (r < 0) {
		fprintf(stderr, "ioctl has failed: %s\n", strerror(errno));
		return -errno;
	} else {
		addr.can_ifindex = ifr.ifr_ifindex;
	}

	r = bind(*can_soc_fd, (struct sockaddr *)&addr, sizeof(addr));

	if (r < 0) {
		fprintf(stderr, "biond has failed: %s\n", strerror(errno));
		return -errno;
	}

	return r;
}
