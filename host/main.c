/* Author: Giuseppe Tipaldi
 *
 */

#include <stdio.h>
#include <getopt.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

#include <net/if.h>

#include "ihbtool.h"

static bool running;

void help(char *prg)
{
	fprintf(stdout, "\nUsage: %s [options] -d CAN interface\n", prg);
	fprintf(stdout, "         -v    Enable the verbose mode\n");
	fprintf(stdout, "         -m    Enable the speed meter\n");
	fprintf(stdout, "Version: %s\n", IHBTOOL_VER);
	fprintf(stdout, "\n");
}

void _signals(const int signo)
{
	switch (signo) {
		case SIGTERM:
			fprintf(stdout, "[*] received signal %i\n", signo);
			running = false;
			break;
		case SIGHUP:
          		fprintf(stdout, "[*] received signal %i\n", signo);
			running = false;
			break;
		case SIGINT:
          		fprintf(stdout, "[*] received signal %i\n", signo);
			running = false;
			break;
		default:
           		break;
	}
}

int main(int argc, char **argv)
{
	bool verbose = false, isotp_fails = false, perf = false;
	int can_soc_raw = 0, can_soc_isotp = 0;
	uint8_t master_id = 255, ihbs_cnt = 0;
	bool assigned_canID[255] = {false};
	char *perph = NULL;
	int c, r = 0;
	bool tryagain = false;

	running = true;

	signal(SIGTERM, _signals);
	signal(SIGHUP, _signals);
	signal(SIGINT, _signals);

	while ((c = getopt(argc, argv, "d:hvm")) != -1) {
		switch (c) {
			case 'd':
				perph = optarg;
				break;
			case 'h':
				help(argv[0]);
				return EXIT_SUCCESS;
				break;
			case 'v':
				verbose = true;
				break;
			case 'm':
				perf = true;
				break;
			case '?':
			default:
				help(argv[0]);
				return EXIT_FAILURE;
		}
	}

	/* validate input */
	if (!perph) {
		fprintf(stderr, BOLDRED"[!] Please specify a CAN device\n"RESET);
                help(argv[0]);
		return EXIT_FAILURE;
	} else if (strlen(perph) > IFNAMSIZ) {
		fprintf(stderr, BOLDRED"[!] name of CAN device '%s' is too long!\n" RESET, perph);
		help(argv[0]);
		return EXIT_FAILURE;
	}

	/* Open and inizializate the socket CAN */
	r = ihb_init_socket_can(&can_soc_raw, perph);
	if (r < 0)
		goto _fail0;

	/* Send wakeup to all IHBs on bus */
	r = ihbs_wakeup(can_soc_raw);
	if (r < 0)
		goto _fail1;
try_again:
	/* Start discovery of the nodes */
	r = ihb_discovery(can_soc_raw, &master_id, &ihbs_cnt, assigned_canID,
			&tryagain, verbose, running);
	if (r < 0)
		goto _fail1;

	if (tryagain) {
		/* Fix IHB nodes which have an can ID collision */
		r = ihb_runtime_fix_collision(can_soc_raw, assigned_canID);
		if (r < 0)
			goto _fail1;

		fprintf(stdout, BOLDCYAN"[*] Start again discovery\n\n"RESET);

		tryagain = false;
		master_id = 255;
		ihbs_cnt = 0;
		ihbs_cleanup();
		goto try_again;
	}

	if(ihbs_cnt != 0) {
		fprintf(stdout, "\n[*] Network size %d. IHB master candidate = %#x\n",
				ihbs_cnt, master_id);
	} else {
		fprintf(stderr, BOLDRED"[!] There are not IHBs available\n"RESET);
		goto _fail1;
	}

	/* Start the setup of the nodes */
	r = ihb_setup(can_soc_raw, master_id, verbose);
	if (r < 0)
		goto  _fail1;

	while (running) {

		/*
		 * To start, the ISO-TP needs a valid IHB node. This logic
		 * assumes as valid only the IHB node with an 0 < ID < 256
		 *
		 * The IHB's fw, implements a fletcher8 (unit8_t) function that
		 * obtains an ID from the MCU's unique ID.
		 *
		 * The next code path is executed when the ihbtool has discovered
		 * the IHBs nodes. The ihbs_cnt counts the IHB.
		 */
		if(ihbs_cnt > 0) {

			/* Open and inizializate the socket CAN ISO-TP */
			r = ihb_init_socket_can_isotp(&can_soc_isotp, perph);
			if (r < 0) {
				break;
			}

			/* Stat the IHB data acquisition */
			r = ihb_rcv_data(can_soc_isotp, verbose, perf, running);

			/*
			 * If the IHB goes in timeout, it has passed away...
			 * The anti fault mechanism covers only ETIMEDOUT
			 */
			if(r == -ETIMEDOUT) {

				/* Close old socket */
				shutdown(can_soc_isotp, 2);
				close(can_soc_isotp);

				isotp_fails = true;
				ihbs_cnt--;

				/* If it was the last exit */
				if(ihbs_cnt == 0) {
					fprintf(stdout, "[*] IHB node=%#x has expired\n",
							master_id);
					fprintf(stdout, "[*] There are not IHBs available\n");
					break;
				}

			} else {
				fprintf(stderr, BOLDRED"[!] IHB stops to send data, err=%d\n"RESET, r);
				/* Stop if any unknown errors happen */
				break;
			}
		}

		if(isotp_fails) {
			r = ihb_blacklist_node(master_id, verbose);
			if (r < 0) {
				fprintf(stderr, BOLDRED"[!] Cannot blacklist the IHB node=%#x\n"RESET, master_id);
				/* Stop if any unknown errors happen */
				break;
			}

			/* Reset the IHB master candidate */
			master_id = 255;

			/* Search for another IHB candidate  */
			r = ihb_discovery_newone(&master_id, verbose);
			if (r < 0)
				break;
		}

		isotp_fails = false;
		fprintf(stdout, "\n[*] Network size %d. IHB master candidate = %#x\n",
				ihbs_cnt, master_id);

		/* Start the setup of the nodes */
		r = ihb_setup(can_soc_raw, master_id, verbose);
		if (r < 0)
			break;
	}

	shutdown(can_soc_isotp, 2);
	close(can_soc_isotp);

_fail1:
	shutdown(can_soc_raw, 2);
	close(can_soc_raw);

	ihbs_cleanup();

_fail0:
	return r;
}
