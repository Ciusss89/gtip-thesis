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

#include "ihb.h"

void help(char *prg)
{
	fprintf(stdout, "\nUsage: %s [options] -d CAN interface\n", prg);
	fprintf(stdout, "         -v    Enable the verbose mode\n");
	fprintf(stdout, "Version: %s\n", IHB_VERSION);
	fprintf(stdout, "\n");
}

void _signals(const int signo)
{
	switch (signo) {
		case SIGTERM:
			fprintf(stderr, "received signal %i\n", signo);
			running = false;
			break;
		case SIGHUP:
          		fprintf(stderr, "received signal %i\n", signo);
			running = false;
			break;
		case SIGINT:
          		fprintf(stderr, "received signal %i\n", signo);
			running = false;
			break;
		default:
           		break;
	}

}

int main(int argc, char **argv)
{
	uint8_t master_id = 255, ihb_nodes = 0;
	bool verbose = false;
	char *perph = NULL;
	int can_soc_raw = 0;
	int can_soc_isotp = 0;
	int c, r = 0;
	void *data;

	running = true;

	signal(SIGTERM, _signals);
	signal(SIGHUP, _signals);
	signal(SIGINT, _signals);

	while ((c = getopt(argc, argv, "d:h:v")) != -1) {
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
			case '?':
			default:
				help(argv[0]);
				return EXIT_FAILURE;
		}
	}

	/* validate input */
	if (!perph) {
		fprintf(stderr, "Please specify a CAN device\n");
                help(argv[0]);
		return EXIT_FAILURE;
	} else if (strlen(perph) > IFNAMSIZ) {
		fprintf(stderr, "name of CAN device '%s' is too long!\n", perph);
		help(argv[0]);
		return EXIT_FAILURE;
	}

	/* Open and inizializate the socket CAN */
	r = ihb_init_socket_can(&can_soc_raw, perph);
	if (r < 0)
		goto _fail;

	/* Start discovery of the nodes */
	r = ihb_discovery(can_soc_raw, verbose, &master_id, &ihb_nodes);
	if (r < 0)
		goto _free_list;

	if(ihb_nodes != 0) {
		fprintf(stdout, "Network size %d. IHB master candidate = %02x\n",
				ihb_nodes, master_id);
	} else {
		puts("IHBs not found");
		goto _fail;
	}

	/* Start the setup of the nodes */
	r = ihb_setup(can_soc_raw, master_id, verbose);
	if (r < 0)
		goto  _free_list;

	 /* Open and inizializate the socket CAN ISO-TP */
	r = ihb_init_socket_can_isotp(&can_soc_isotp, perph, master_id);
	if (r < 0)
		 goto  _free_list;

	while (running && ihb_nodes > 0) {
		r = ihb_rcv_data(can_soc_isotp, &data, verbose);

		/* If the IHB goes in timeout, it has passed away */
		if(r == -ETIMEDOUT) {
			// !TODO Add logic
			//  1. to search new candidate
			//  2. to configure new candidate
		} else {
			/* The Fault mechanism covers only ETIMEDOUT */
			break;
		}

		ihb_nodes--;
		fprintf(stdout, "Network size %d. IHB master candidate = %02x\n",
				ihb_nodes, master_id);
	
		/* Close old socket */
		shutdown(can_soc_isotp, 2);
		close(can_soc_isotp);

		 /* Open and inizializate the socket CAN ISO-TP */
		r = ihb_init_socket_can_isotp(&can_soc_isotp, perph, master_id);
		if (r < 0)
			 running = false;
	}

	HASH_CLEAR(hh,ihbs);

	shutdown(can_soc_isotp, 2);
	close(can_soc_isotp);

	shutdown(can_soc_raw, 2);
	close(can_soc_raw);

_free_list:
	HASH_CLEAR(hh,ihbs);

_fail:

	return r;
}
