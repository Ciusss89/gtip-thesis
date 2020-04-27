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
#include <linux/sockios.h>

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

	fprintf(stderr, BOLDCYAN"[@] BUG: Node %#x not in list\n"RESET, ihb->canID);

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

	fprintf(stderr, BOLDCYAN"[@] BUG: fallback fails..\n"RESET);
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
					fprintf(stderr, BOLDRED"[!] asprintf fails\n"RESET);
					break;
				}
				ihb->best = true;
			} else {
				/* Send ASCII: IHB-BUSY to idle nodes */
				r = asprintf(&cmd, "%03x#4948422D49444C45",
					     ihb->canID);
				if (r < 0) {
					fprintf(stderr, BOLDRED"[!] asprintf fails\n"RESET);
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
			fprintf(stderr, BOLDRED"[!] can_soc_fd not ready: %s\n"RESET,
					strerror(errno));
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
				fprintf(stderr, BOLDRED"[!] read failure: %s\n"RESET,
						strerror(errno));
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
						fprintf(stderr, BOLDRED"[!] malloc failure:\n"RESET);
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
		fprintf(stderr, BOLDRED"[!] socket open failure! %s\n"RESET,
				strerror(errno));
		return -errno;
	}

	addr.can_family = AF_CAN;
	memset(&ifr.ifr_name, 0, sizeof(ifr.ifr_name));
	strncpy(ifr.ifr_name, d, sizeof(d) + 1);

	r = ioctl(*can_soc_fd, SIOCGIFINDEX, &ifr);
	if (r < 0) {
		fprintf(stderr, BOLDRED"[!] ioctl has failed: %s\n"RESET, strerror(errno));
		return -errno;
	} else {
		addr.can_ifindex = ifr.ifr_ifindex;
	}

	r = bind(*can_soc_fd, (struct sockaddr *)&addr, sizeof(addr));

	if (r < 0) {
		shutdown(*can_soc_fd, 2);
		close(*can_soc_fd);

		fprintf(stderr, BOLDRED"[!] bind has failed: %s\n"RESET, strerror(errno));
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
		fprintf(stderr, BOLDRED"[!] socket open failure! %s\n"RESET, strerror(errno));
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
		fprintf(stderr, BOLDRED"[!] bind has failed: %s\n"RESET,
				strerror(errno));
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
		fprintf(stderr, BOLDRED"bind has failed: %s\n"RESET,
				strerror(errno));
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
		fprintf(stderr, BOLDRED"[!] bind has failed: %s\n"RESET,
				strerror(errno));
		return -errno;
	}

	return r;
}

static void ihb_info_print(struct ihb_node_info *info)
{
	puts("*------------------------------------------------");
	printf("| RIOT-OS version %s\n", info->riotos_ver);
	printf("| IHB fw version  %s\n", info->ihb_fw_rev);
	printf("| IHB board name  %s\n", info->mcu_board);
	printf("| IHB mcu arch    %s\n", info->mcu_arch);
	printf("| IHB board uid   %s\n", info->mcu_uid);
	printf("| IHB skin nodes  %d\n", info->skin_nodes);
	printf("| SKIN tactiles   %d\n", info->skin_tactails);
	printf("| ISO TP timeout  %u\n", info->isotp_timeo);
	puts("*------------------------------------------------");
}

static void ihb_sk_nodes_print(struct skin_node *sk_nodes, size_t sk, size_t tcl)
{
	uint8_t i, j;

	puts("\n-----------------------------------DEBUG---------------------------------------");
	for(i = 0; i < sk; i++) {
		printf("%-3d [%#x] TS: ", i, sk_nodes[i].address);
		for(j = 0; j < tcl; j++)
			printf(" %02x", sk_nodes[i].data[j]);
		printf(" Skin Node = %s\n", sk_nodes[i].expired ? "offline" : "alive");
	}
	puts("-------------------------------------------------------------------------------");
}

static void ihb_isotp_perf(struct timeval _end_tv,
			   struct timeval _start_tv,
			   int bytes,
			   char **_buff,
			   bool perf)
{
	struct timeval diff_tv;
	char *buff = NULL;

	if(!perf) {
		asprintf(&buff, " ");
		*_buff = buff;
		return;
	}

	/* calculate time */
	diff_tv.tv_sec  = _end_tv.tv_sec  - _start_tv.tv_sec;
	diff_tv.tv_usec = _end_tv.tv_usec - _start_tv.tv_usec;

	//printf("---><%ld.%06ld>", (long int)(diff_tv.tv_sec), (long int)(diff_tv.tv_usec));

	if (diff_tv.tv_usec < 0)
		diff_tv.tv_sec--, diff_tv.tv_usec += 1000000;
	if (diff_tv.tv_sec < 0)
		diff_tv.tv_sec = diff_tv.tv_usec = 0;

	if (diff_tv.tv_sec * 1000 + diff_tv.tv_usec / 1000){
		asprintf(&buff, " time %ld.%06lds, => %lu byte/s ", diff_tv.tv_sec, diff_tv.tv_usec,
				(bytes * 1000) /
				(diff_tv.tv_sec * 1000 + diff_tv.tv_usec / 1000));
	} else {
		asprintf(&buff, "(no time available)");
	}

	*_buff = buff;

	return;
}

int ihb_rcv_data(int fd, void **ptr, bool v, bool perf)
{
	const size_t info_size = sizeof(struct ihb_node_info);
	static struct ihb_node_info *ihb_node = NULL;

	const size_t nmemb = sizeof(struct skin_node);
	static struct skin_node *sk_nodes = NULL;

	struct timeval timeout_config, start_tv, end_tv;

	size_t buff_l = 0, sk_nodes_count = 0, sk_tacts = 0;
	int r, z, nbytes;
	bool info_received = false;
	char *buff = NULL;
	void *p = NULL;
	fd_set rdfs;

	z = 0;

	do {
		/* Initial timeout for ISO TP transmissions */
		timeout_config.tv_sec  = 5;
		timeout_config.tv_usec = 0;

		FD_ZERO(&rdfs);
		FD_SET(fd, &rdfs);

		r = select(fd + 1, &rdfs, NULL, NULL, &timeout_config);

		if (r < 0) {
			fprintf(stderr, BOLDRED"[!]socket not ready: %s\n"RESET,
					strerror(errno));
			break;
		}

		if (r == 0) {
			r = -ETIMEDOUT;
			fprintf(stdout, RED"\n[*] IHB failure detected: %s\n"RESET,
					strerror(ETIMEDOUT));
			break;
		}

		if (FD_ISSET(fd, &rdfs)) {

			/* IHB sends as first the info package. */
			if (info_received) {

				p = calloc(SK_N_S, nmemb);
				if(!p){
					fprintf(stderr, BOLDRED"[!] calloc fails\n"RESET);
					return -1;
				}

				ioctl(fd, SIOCGSTAMP, &start_tv);
				nbytes = read(fd, p, buff_l);
				ioctl(fd, SIOCGSTAMP, &end_tv);

				if(nbytes != buff_l) {
					fprintf(stdout, RED"[!] short rcv, ignore cunks #%d\n"RESET, z);
					free(p);
				}

				if (p) {
					sk_nodes = (struct skin_node *)p;

					if(v)
						ihb_sk_nodes_print(sk_nodes, sk_nodes_count, sk_tacts);

					ihb_isotp_perf(end_tv, start_tv, nbytes, &buff, perf);

					fprintf(stdout, "\r[*] IHB is sending data: chunk=%ubytes counts=%d%s",
						nbytes,
						z,
						buff);

					z++;

					free(p);
					free(buff);
				}

			} else {

				p = malloc(info_size);
				if(!p){
					fprintf(stderr, BOLDRED"[!] malloc fails\n"RESET);
					return -1;
				}

				nbytes = read(fd, p, info_size);
				if(nbytes != info_size) {
					fprintf(stdout, RED"[!] short rcv\n"RESET);
					free(p);
				}

				if (p) {
					ihb_node = (struct ihb_node_info *)p;

					if (ihb_node->skin_nodes < MAX_SK_NODES)
						sk_nodes_count = ihb_node->skin_nodes;

					if (ihb_node->skin_tactails < MAX_SK_TACTAILS)
						sk_tacts = ihb_node->skin_tactails;

					timeout_config.tv_sec = ihb_node->isotp_timeo;

					buff_l = nmemb * sk_nodes_count;
					ihb_info_print(ihb_node);
					free(p);

					/*
					 * Switch to collect the incoming data
					 * from IHB
					 */
					info_received = true;
				}
			}
		}

	} while (running);


	fprintf(stdout, "\n[!] Receiving data from IHB is ended.\n");
	return r;
}
