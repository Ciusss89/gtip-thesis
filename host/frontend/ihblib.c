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
#include <linux/can/isotp.h>

#include "ihb.h"

/* IHB device */
#include "ihb-dev.h"

/* CAN-UTILS lib */
#include "lib.h"

#define BUFSIZE 5000 /* size > 4095 to check socket API internal checks */

static struct ihb_node *find_canID(int can_id) {
	struct ihb_node *s;

	HASH_FIND_INT(ihbs, &can_id, s);  /* s: output pointer */
	return s;
}

int ihb_blacklist_node(uint8_t ihb_expired, bool v)
{
	struct ihb_node *ihb;
	bool find = false;

	for(ihb = ihbs; ihb != NULL; ihb = (struct ihb_node *)(ihb->hh.next)) {
		if(v)
			fprintf(stdout, "[#] Checking IHB node=%#x\n", ihb->canID);

		if(ihb->canID == ihb_expired){
			fprintf(stdout, "[*] IHB node=%#x has expired\n",
					ihb->canID);
			ihb->expired = true;
			find = true;
		}
	}
	
	if(find)
		return 0;

	fprintf(stderr, "[@] BUG: Node %#x not in list\n", ihb->canID);

	return -1;
}


int ihb_discovery_newone(uint8_t *master_id, bool v)
{
	struct ihb_node *ihb;
	bool find = false;

	for(ihb = ihbs; ihb != NULL; ihb = (struct ihb_node *)(ihb->hh.next)) {
		if(ihb->expired){
			if(v)
				fprintf(stdout, "[#] ignore IHB candidate %#x\n", ihb->canID);
			continue;
		} else {
			if(ihb->canID <= *master_id) {
				*master_id = ihb->canID;

				if(v)
					fprintf(stdout, "[#] IHB new candidate %#x\n", ihb->canID);

				find = true;
			}
		}
	}

	if(find) {
		fprintf(stdout, "[*] Fallback on the IHB node=%#x\n", *master_id);
		return 0;
	}

	fprintf(stderr, "[@] BUG: fallback fails..\n");
	return -1;
}

int ihb_setup(int s, uint8_t c_id_master, bool v)
{
	struct canfd_frame frame;
	struct ihb_node *ihb;

	int r = -1, required_mtu = -1;
	char *cmd;

	for(ihb = ihbs; ihb != NULL; ihb = (struct ihb_node *)(ihb->hh.next)) {

		if(!ihb->expired) {

			if(ihb->canID == c_id_master) {
				/* Send ASCII: IHB-BUSY to master node */
				r = asprintf(&cmd, "%03x#4948422D42555359",
					     ihb->canID);
				if (r < 0) {
					fprintf(stderr, "[!] asprintf fails\n");
					break;
				}
				ihb->best = true;
			} else {
				/* Send ASCII: IHB-BUSY to idle nodes */
				r = asprintf(&cmd, "%03x#4948422D49444C45",
					     ihb->canID);
				if (r < 0) {
					fprintf(stderr, "[!] asprintf fails\n");
					break;
				}
				ihb->best = false;
			}

			/* ASCII message are both 8 bytes length */
			frame.len = 8;

			/*
			 * Parse CAN frame
			 *
			 * Transfers a valid ASCII string describing a CAN frame
			 * into struct canfd_frame
			 */
			required_mtu = parse_canframe(cmd, &frame);

			/* send frame */
			if (write(s, &frame, required_mtu) != required_mtu) {
				r = -1;
				break;
			}

			free(cmd);

			fprintf(stdout, "[*] Configuring the IHB node=%#x %s\n",
					ihb->canID,
					ihb->best ? "as MASTER" : "as FALLBACK");
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


	while (discovery && running) {
		FD_ZERO(&rdfs);
		FD_SET(fd, &rdfs);

		r = select(fd + 1, &rdfs, NULL, NULL, &timeout_config);

		if (r < 0) {
			fprintf(stderr, "[!] can_soc_fd not ready: %s\n", strerror(errno));
			discovery = false;
			continue;
		}

		if (r == 0) {
			fprintf(stdout, "[*] Discovery of IHBs has been elapsed\n");
			discovery = false;
			continue;
		}

		if (FD_ISSET(fd, &rdfs)) {

			recv_bytes = read(fd, &frame_rd, sizeof(struct can_frame));
			if (!recv_bytes) {
				fprintf(stderr, "[!] read failure: %s\n", strerror(errno));
				continue;
			}

			if(v) {
				fprintf(stdout, "id = %#x  dlc = [%d], data = ",
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
						fprintf(stderr, "[!] malloc failure:\n");
						discovery = false;
						return -1;
					}

					ihb->expired = false;
					ihb->canID = frame_rd.can_id;
					ihb->canP = NULL; /* not used for now */

					HASH_ADD_INT(ihbs, canID, ihb);

					fprintf(stdout, "[*] IHB node=%#x has been added\n",
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
		fprintf(stderr, "[!] socket open failure! %s\n", strerror(errno));
		return -errno;
	}

	addr.can_family = AF_CAN;
	memset(&ifr.ifr_name, 0, sizeof(ifr.ifr_name));
	strncpy(ifr.ifr_name, d, sizeof(d) + 1);

	r = ioctl(*can_soc_fd, SIOCGIFINDEX, &ifr);
	if (r < 0) {
		fprintf(stderr, "[!] ioctl has failed: %s\n", strerror(errno));
		return -errno;
	} else {
		addr.can_ifindex = ifr.ifr_ifindex;
	}

	r = bind(*can_soc_fd, (struct sockaddr *)&addr, sizeof(addr));

	if (r < 0) {
		shutdown(*can_soc_fd, 2);
		close(*can_soc_fd);

		fprintf(stderr, "[!] bind has failed: %s\n", strerror(errno));
		return -errno;
	}

	return r;
}

int ihb_init_socket_can_isotp(int *can_soc_fd, const char *d)
{
	static struct can_isotp_fc_options fcopts;
	static struct can_isotp_options opts;
	struct sockaddr_can addr;

	int r;

	/*
	 * !TODO: add tuning of the fc options:
	 *
	 * fcopts.bs =
	 * fcopts.stmin =
	 * fcopts.wftmax =
	 */

	*can_soc_fd = socket(PF_CAN, SOCK_DGRAM, CAN_ISOTP);
	if (*can_soc_fd < 0) {
		fprintf(stderr, "[!] socket open failure! %s\n", strerror(errno));
		return -errno;
	}

	r = setsockopt(*can_soc_fd,
		       SOL_CAN_ISOTP,
		       CAN_ISOTP_OPTS,
		       &opts,
		       sizeof(opts));
	if (r < 0) {
		shutdown(*can_soc_fd, 2);
		close(*can_soc_fd);
		fprintf(stderr, "[!] bind has failed: %s\n", strerror(errno));
		return -errno;
	}

	r = setsockopt(*can_soc_fd,
		       SOL_CAN_ISOTP,
		       CAN_ISOTP_RECV_FC,
		       &fcopts,
		       sizeof(fcopts));
	if (r < 0) {
		shutdown(*can_soc_fd, 2);
		close(*can_soc_fd);
		fprintf(stderr, "bind has failed: %s\n", strerror(errno));
		return -errno;
	}

	addr.can_family = AF_CAN;
	addr.can_ifindex = if_nametoindex(d);
	addr.can_addr.tp.tx_id = 0x708;
	addr.can_addr.tp.rx_id = 0x700;

	r = bind(*can_soc_fd, (struct sockaddr *)&addr, sizeof(addr));

	if (r < 0) {
		shutdown(*can_soc_fd, 2);
		close(*can_soc_fd);
		fprintf(stderr, "[!] bind has failed: %s\n", strerror(errno));
		return -errno;
	}

	return r;
}

int ihb_rcv_data(int fd, void **ptr, bool v)
{
	const size_t nmemb = sizeof(struct skin_node);
	static struct skin_node *sk_nodes = NULL;
	const size_t buff_l = SK_N_S * nmemb;
	int r, z, i, j, nbytes;
	void *p = NULL;
	fd_set rdfs;

	z = 0;

	do {
		struct timeval timeout_config = { 5, 0 };
		FD_ZERO(&rdfs);
		FD_SET(fd, &rdfs);

		r = select(fd + 1, &rdfs, NULL, NULL, &timeout_config);

		if (r < 0) {
			fprintf(stderr, "[!]socket not ready: %s\n", strerror(errno));
			break;
		}

		if (r == 0) {
			r = -ETIMEDOUT;
			fprintf(stdout, "\n[*] IHB failure detected: %s\n", strerror(ETIMEDOUT));
			break;
		}

		if (FD_ISSET(fd, &rdfs)) {
			p = calloc(SK_N_S, nmemb);
			if(!p){
				fprintf(stderr, "[!] calloc fails\n");
				return -1;
			}

			nbytes = read(fd, p, buff_l);

			if(nbytes != buff_l) {
				fprintf(stdout, "[!] short rcv, ignore cunks #%d\n", z);
				goto _short_rcv;
			}

			sk_nodes = (struct skin_node *)p;

			if(v) {
				puts("\n-----------------------------------DEBUG---------------------------------------");
				for(i = 0; i < SK_N_S; i++) {
					printf("%-3d [%#x] TS: ", i, sk_nodes[i].address);
					for(j = 0; j < SK_T_S; j++)
						printf(" %02x", sk_nodes[i].data[j]);
					printf(" Skin Node = %s\n", sk_nodes[i].expired ? "offline" : "alive");
				}
				puts("-------------------------------------------------------------------------------");
			}

			printf("\r[*] IHB is sending data: chunk=%ubytes counts=%d", nbytes, z);
			z++;
_short_rcv:
			free(p);
		}

	} while (running);


	puts("\n[!] Receiving data from IHB is ended.\n");
	return r;
}
