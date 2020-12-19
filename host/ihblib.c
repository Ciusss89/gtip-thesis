#define _GNU_SOURCE
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
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

#include "ihbtool.h"

/* The ihb.h and message.h are simbolik links to firmware libs*/
#include "ihb.h"
#include "message.h"

/* CAN-UTILS lib */
#include "lib.h"

#define BUFSIZE 5000 /* size > 4095 to check socket API internal checks */

struct ihb_node *ihbs = NULL;

static struct ihb_node *find_canID(int can_id) {
	struct ihb_node *s;

	HASH_FIND_INT(ihbs, &can_id, s);  /* s: output pointer */
	return s;
}

void ihbs_cleanup()
{
	HASH_CLEAR(hh,ihbs);
	free(ihbs);
	ihbs = NULL;
}

static int send_msg(int fd, char *msg)
{
	struct canfd_frame frame;
	int required_mtu = -1;
	int attemp = 10;

	/*
	 * Parse CAN frame
	 *
	 * Transfers a valid ASCII string describing a CAN frame
	 * into struct canfd_frame
	 */
	required_mtu = parse_canframe(msg, &frame);

	/* Send the msg each 20ms for 50 times */
	while(attemp > 0) {
		/* send frame */
		if (write(fd, &frame, required_mtu) != required_mtu) {
			fprintf(stderr, BOLDRED"[!] write fails\n"RESET);
			return -1;
		}
		attemp--;
		usleep(200);
		
		/*  TODO: should be sent until IHB doesn't send me ack */
	}

	return 0;
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

int ihbs_wakeup(int s)
{
	char *cmd;
	int r;

	r = asprintf(&cmd, "%03x#%s", IHBTOLL_FRAME_ID, msg_wkup);
	if (r < 0) {
		fprintf(stderr, BOLDRED"[!] asprintf fails\n"RESET);
		return -1;
	}

	if (send_msg(s, cmd) < 0) {
		free(cmd);
		return -1;
	}

	free(cmd);
	return 0;
}

int ihb_setup(int s, uint8_t c_id_master, bool v)
{
	struct ihb_node *ihb;
	int r = -1;
	char *cmd;

	for(ihb = ihbs; ihb != NULL; ihb = (struct ihb_node *)(ihb->hh.next)) {

		if(!ihb->expired) {

			if(ihb->canID == c_id_master) {
				r = asprintf(&cmd, "%03x#%s", ihb->canID, msg_mstr);
				if (r < 0) {
					fprintf(stderr, BOLDRED"[!] asprintf fails\n"RESET);
					break;
				}
				ihb->best = true;
			} else {
				r = asprintf(&cmd, "%03x#%s", ihb->canID, msg_bckp);
				if (r < 0) {
					fprintf(stderr, BOLDRED"[!] asprintf fails\n"RESET);
					break;
				}
				ihb->best = false;
			}

			r = send_msg(s, cmd);
			if (r < 0) {
				free(cmd);
				break;
			}
			free(cmd);

			fprintf(stdout, "[*] Configuring the IHB node=%#x which ends whit %#x %s\n",
					ihb->canID,
					ihb->uid_LSBytes[0],
					ihb->best ? "as ACTIVE" : "as BACKUP");
		}
		usleep(20);
	}

	return r;
}


static uint8_t ihb_get_new_can_id(bool *assigned_canID)
{
	uint8_t i = 254;

	for(; i > 0; i--) {
		/* IF false, the CAN ID is free */
		if(assigned_canID[i] == false) {
			assigned_canID[i] = true;
			break;
		}
	}

	return i;

}

static void ihbs_clean(bool *assigned_canID)
{
	struct ihb_node *ihb = ihbs;
	uint8_t i = 0;

	for(; ihb != NULL; ihb = (struct ihb_node *)(ihb->hh.next)) {
		memset(ihb->uid_LSBytes, 0, 255 * sizeof(uint16_t));
		HASH_DEL(ihbs, ihb);
	}

	for (; i < 255; i++)
		assigned_canID[i] = false;

	return;
}

int ihb_runtime_fix_collision(int fd, bool *assigned_canID)
{
	struct ihb_node *ihb;
	uint8_t new_canID;
	char *cmd;
	int r = 0;

	/* For each IHB CAN id*/
	for(ihb = ihbs; ihb != NULL && r >= 0; ihb = (struct ihb_node *)(ihb->hh.next)) {
		/* For each IHB which needs address fix */
		for(int j = 1; j <= ihb->cnt; j ++) {
			new_canID = ihb_get_new_can_id(assigned_canID);
			/*
			 *                      ┌ New CAN frame id
			 * ASCI Message: IHB-XX=Y
			 *                   ||
			 *        LSBytes[0] ┘└ LSBytes[1]
			 *
			 * Fix runtime the addressing, last byte contains the new
			 * frame identifier which must be used
			 */
			r = asprintf(&cmd, "%03x#%s%04x3D%02x", ihb->canID,
								msg_add_fix,
								ihb->uid_LSBytes[j],
								new_canID);
			if (r < 0) {
				fprintf(stderr, BOLDRED"[!] asprintf fails\n"RESET);
				break;
			}

			fprintf(stdout, CYAN"[*] Add IHB=%#x which ends with %#x will use %#x\n"RESET,
				ihb->canID, ihb->uid_LSBytes[ihb->cnt], new_canID);

			if (send_msg(fd, cmd) < 0) {
				free(cmd);
				r = -1;
				break;
			}
			free(cmd);
		}
	}
	
	ihbs_clean(assigned_canID);
	return r;
}


static void ihb_canID_collision(struct ihb_node *ihb, uint16_t current_new,
				uint8_t current_canID, bool *try_again)
{

	if (current_canID != ihb->canID)
		return;

	/*
	 * Return if the current_new is the node which has been handled
	 * by ihb_add_new_node
	 */
	if (current_new == ihb->uid_LSBytes[0])
		return;

	/*
	 * If a collision happens the IHB nodes which have the same CAN ID and
	 * different uid_LSBytes must be add to the reconfigure list.
	 */
	for(int j = 1; j < 255; j ++) {
		/* Has it been added? */
		if (ihb->uid_LSBytes[j] == current_new)
			return;
	}

	ihb->cnt = ihb->cnt + 1;
	ihb->uid_LSBytes[ihb->cnt] = current_new;
	fprintf(stdout, YELLOW"[!] Add IHB=%#x ending with %#x to reconfigure list, total %d\n"RESET,
				ihb->canID, ihb->uid_LSBytes[ihb->cnt], ihb->cnt);

	/* True means that ihb_discovery must be lunched again */
	*try_again = true;
}

static int ihb_add_new_node(struct ihb_node **out, struct can_frame fr,
			    uint8_t *ihbs_cnt, bool *assigned_canID)
{
	struct ihb_node *ihb;

	ihb = malloc(sizeof(struct ihb_node));
	if (!ihb) {
		fprintf(stderr, BOLDRED"[!] malloc failure:\n"RESET);
		return -1;
	}

	/* Save:
	 * 1. two LSBytes of mcu id
	 * 2. IHB can id
	 */
	ihb->uid_LSBytes[0] = ((uint16_t)fr.data[6] << 8) | fr.data[7];
	ihb->expired = false;
	ihb->canID = fr.can_id;
	ihb->cnt = 0;

	fprintf(stdout, "[*] IHB node=%#x which ending serial number with %#x has been added\n",
			ihb->canID, ihb->uid_LSBytes[0]);

	/* Assigned_canID is an array of 256 bool used to trak the canID */
	assigned_canID[ihb->canID] = true;

	/* Update counter of IHB nodes */
	*ihbs_cnt = *ihbs_cnt + 1;

	*out = ihb;
	return 0;
}

static void print_raw_frame(struct can_frame fr)
{
	fprintf(stdout, "id = %#x  dlc = [%d], data = ", fr.can_id, fr.can_dlc);
		for (int i = 0; i < fr.can_dlc; i++)
				printf("%02x", fr.data[i]);
		printf("\n");

}

int ihb_discovery(int fd, uint8_t *best, uint8_t *ihbs_cnt,
		  bool *assigned_canID, bool *try_again,
		  bool verbose, bool running)
{
	struct timeval timeout_config = { 15, 0 };
	struct can_frame frame_rd;
	struct ihb_node *ihb;
	ssize_t recv_bytes = 0;
	bool discovery = true;
	uint16_t cur_uid;
	fd_set rdfs;
	int r = -1;


	while (discovery && running) {
		FD_ZERO(&rdfs);
		FD_SET(fd, &rdfs);

		r = select(fd + 1, &rdfs, NULL, NULL, &timeout_config);

		if (r < 0) {
			fprintf(stderr, BOLDRED"[!] can_soc_fd not ready: %s\n"RESET,
					strerror(errno));
			discovery = false;
			ihbs_clean(assigned_canID);
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

			if(verbose)
				print_raw_frame(frame_rd);

			if (memcmp(frame_rd.data, IHBMAGIC, 6) == 0 &&
				   frame_rd.can_dlc == 8) {

				cur_uid = ((uint16_t)frame_rd.data[6] << 8) |
					  frame_rd.data[7];
				ihb = find_canID(frame_rd.can_id);

				if (!ihb) {
					/* New IHB node with a new can ID */
					r = ihb_add_new_node(&ihb, frame_rd, ihbs_cnt, assigned_canID);
					if (r < 0) {
						discovery = false;
						ihbs_clean(assigned_canID);
						if(ihb)
							free(ihb);
						break;
					}
					HASH_ADD_INT(ihbs, canID, ihb);
				} else {
					/* New IHB node with the same can ID */
					ihb_canID_collision(ihb, cur_uid, frame_rd.can_id, try_again);
				}

				if (frame_rd.can_id <= *best)
					*best = frame_rd.can_id;
			}

		} else {
			discovery = false;
			ihbs_clean(assigned_canID);
			r = -1;
			break;
		}
	}

	fprintf(stderr, "[*] CAN ID discovered: ");
	for (int i = 0; i <= 255; i++) {
		if (assigned_canID[i] != false)
			 fprintf(stderr, "0x%02x ", i);
	}
	fprintf(stderr, "\n");

	return r;
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
	addr.can_addr.tp.tx_id = ISOTP_IHB_RX_PORT;
	addr.can_addr.tp.rx_id = ISOTP_IHB_TX_PORT;

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
	printf("| IHB skin nodes  %d\n", info->skin_node_count);
	printf("| SKIN tactiles   %d\n", info->skin_node_taxel);
	printf("| ISO TP timeout  %u\n", info->isotp_timeo);
	puts("*------------------------------------------------");
}

static void ihb_sk_nodes_print(struct skin_node *sk_nodes, size_t sk, size_t tcl)
{
	uint8_t i, j;

	puts("\n-----------------------------------DEBUG---------------------------------------");
	for(i = 0; i < sk; i++) {
		fprintf(stdout, "%-3d [%#x] TS: ", i, sk_nodes[i].address);

		/* Print tactile if skin node is alive  */
		if (!sk_nodes[i].expired) {
			for(j = 0; j < tcl; j++)
				printf(" %02x", sk_nodes[i].data[j]);
		} else {
			fprintf(stdout, " -- (no data available) -- ");
		}

		fprintf(stdout, " Skin Node = %s\n", sk_nodes[i].expired ?
				RED"offline"RESET : "alive");
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
	int r;

	if(!perf) {
		r = asprintf(&buff, " ");
		if (r < 0) {
			fprintf(stderr, BOLDRED"[!] asprintf fails\n"RESET);
			return;
		}

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
		r = asprintf(&buff, " time %ld.%06lds, => %lu byte/s ", diff_tv.tv_sec, diff_tv.tv_usec,
				    (bytes * 1000) /
				    (diff_tv.tv_sec * 1000 + diff_tv.tv_usec / 1000));
		if (r < 0) {
			fprintf(stderr, BOLDRED"[!] asprintf fails\n"RESET);
			return;
		}
	} else {
		r = asprintf(&buff, "(no time available)");
		if (r < 0) {
			fprintf(stderr, BOLDRED"[!] asprintf fails\n"RESET);
			return;
		}
	}

	*_buff = buff;

	return;
}

static void ihb_sk_notify_fails(struct skin_node *sk_nodes, size_t sk, bool *sk_node_expi_notified)
{
	uint8_t i;

	for(i = 0; i < sk; i++) {
		if(sk_nodes[i].expired && !sk_node_expi_notified[i]) {

			/* Notify the failure only the first time */
			sk_node_expi_notified[i] = true;

			fprintf(stdout, RED"\nSkin Node [%#x] has expired\n"RESET,
					sk_nodes[i].address);
		}
	}

	return;
}

int ihb_rcv_data(int fd, bool verbose, bool perf, bool running)
{
	const size_t info_size = sizeof(struct ihb_node_info);
	static struct ihb_node_info *ihb_node = NULL;

	const size_t str_sk_size = sizeof(struct skin_node);
	static struct skin_node *sk_nodes = NULL;

	struct timeval timeout_config, start_tv, end_tv;

	size_t buff_l = 0, sk_nodes_count = 0, sk_tacts = 0;
	bool ihb_info_received = false;
	bool *sk_nodes_ex_vect = NULL;
	char *buff = NULL;
	void *p = NULL;
	int r, cnt;
	fd_set rdfs;

	cnt = 0;

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
			if (ihb_info_received) {

				/* Collect the data which are incoming by IHB */

				p = calloc(sk_nodes_count, str_sk_size);
				if(!p){
					fprintf(stderr, BOLDRED"[!] calloc fails\n"RESET);
					r = -ENOMEM;
					break;
				}

				ioctl(fd, SIOCGSTAMP, &start_tv);
				r = read(fd, p, buff_l);
				ioctl(fd, SIOCGSTAMP, &end_tv);
				cnt++;

				if(r != buff_l) {
					fprintf(stdout, RED"[!] short rcv, ignore cunks #%d\n"RESET, cnt);
					free(p);
					p = NULL;
					/*
					 * This error is not critical, if
					 * happens means that the received
					 * chunk is corrupted.
					 *
					 * Skip it
					 */
				}

				if (p) {
					sk_nodes = (struct skin_node *)p;

					if (verbose)
						ihb_sk_nodes_print(sk_nodes, sk_nodes_count, sk_tacts);
					else
						ihb_sk_notify_fails(sk_nodes, sk_nodes_count, sk_nodes_ex_vect);

					/*
					 * When perf is true saves into &buff
					 * the speed stats.
					 */
					ihb_isotp_perf(end_tv, start_tv, r, &buff, perf);

					/* Update stdout */
					fprintf(stdout, "\r[*] IHB is sending data: chunk=%ubytes counts=%d%s",
						r,
						cnt,
						buff);

					free(p);
					free(buff);
				}

			} else {

				p = malloc(info_size);
				if(!p){
					fprintf(stderr, BOLDRED"[!] malloc fails\n"RESET);
					r = -ENOMEM;
					break;
				}

				r = read(fd, p, info_size);
				if(r != info_size) {
					fprintf(stdout, RED"[!] short rcv\n"RESET);
					free(p);
					r = -EIO;
					break;
				}

				if (p) {
					ihb_node = (struct ihb_node_info *)p;

					if (ihb_node->skin_node_count <= SKIN_MAX_COUNT)
						sk_nodes_count = ihb_node->skin_node_count;
					else {
						fprintf(stderr, BOLDRED"[!] MAX_SK_NODES=%d\n"RESET,
								SKIN_MAX_COUNT);
						r = -EOVERFLOW;
						break;
					}

					if (ihb_node->skin_node_taxel <= SK_T_S)
						sk_tacts = ihb_node->skin_node_taxel;
					else {
						fprintf(stderr, BOLDRED"[!] MAX_SK_TACTAILS=%d\n"RESET,
								SK_T_S);
						r = -EOVERFLOW;
						break;
					}

					/* Receive the iso-tp timeout from IHB */
					timeout_config.tv_sec = ihb_node->isotp_timeo;

					/*
					 * The buff_l will be equal to the size
					 * of the chunk sent by IHB
					 */
					buff_l = str_sk_size * sk_nodes_count;

					ihb_info_print(ihb_node);
					free(p);

					/*
					 * Switch to collect the incoming data
					 * from IHB
					 */
					ihb_info_received = true;

					/*
					 * Build an array of bool which is used
					 * to track the SKIN nodes that have
					 * passed away.
					 */
					sk_nodes_ex_vect = (void *)calloc(1, sk_nodes_count);
					if (!sk_nodes_ex_vect){
						fprintf(stderr, BOLDRED"[!] calloc fails\n"RESET);
						r = -ENOMEM;
						break;
					}
				}
			}
		}
	} while (running);

	if (sk_nodes_ex_vect)
		free(sk_nodes_ex_vect);

	fprintf(stdout, "\n[!] Receiving data from IHB is ended.\n");
	return r;
}
