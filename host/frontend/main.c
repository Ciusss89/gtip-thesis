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

#include <net/if.h>

#include "ihb.h"

static bool running = true;

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
	int can_soc_fd = 0;
	int c, r;

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
	r = ihb_init_socket_can(&can_soc_fd, perph);
	if (r < 0)
		goto err_init;

	/* Start discovery of the nodes */
	r = ihb_discovery(can_soc_fd, verbose, &master_id, &ihb_nodes);
	if (r < 0)
		goto  err_discovery;

	if(ihb_nodes != 0)
		fprintf(stdout, "Network size %d. IHB master candidate = %02x\n",
				ihb_nodes, master_id);

	/* Start the setup of the nodes */
	//struct canfd_frame frame;
	//int required_mtu;
	//required_mtu = parse_canframe(perph, &frame);
	r = ihb_setup(can_soc_fd, master_id, verbose);
	if (r < 0)
		goto  err_setup;

	/*
	 * NEXT STEP,
	 *   
	 *   master_id send MASTER REQUEST.
	 *
	 *
	 * */

err_setup:
	HASH_CLEAR(hh,ihbs);

err_discovery:

err_init:
	close(can_soc_fd);

	return r;
}
