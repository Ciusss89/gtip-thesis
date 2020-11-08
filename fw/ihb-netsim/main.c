/**
 * Author:	Giuseppe Tipaldi
 * Created:	2020
 **/
#define _GNU_SOURCE

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

/* RIOT APIs */
#include "board.h"
#include "random.h"
#include "thread.h"
#include "xtimer.h"

#define ENABLE_DEBUG    (0)
#include "debug.h"

#include "ihb-tools/tools.h"
#include "skin.h"
#include "ihb.h"

#define SK_THREAD_HELP "ihb-netsim thread"

static char skin_sim_stack[THREAD_STACKSIZE_MEDIUM];

#ifdef MODULE_IHBCAN
#include "ihb-can/can.h"
#endif

const size_t data_bs = sizeof(struct skin_node);
const size_t buffer_len = SK_N_S * data_bs;
static struct skin_node *sk = NULL;
struct ihb_ctx *IHB = NULL;
uint8_t us_fail_node = 255;

static void _usage(void)
{
	puts("SKIN nodes simulator userspace");
	puts("\tskin fail <node> - set as expired the node");
}

int skin_node_handler(int argc, char **argv)
{
	if (argc < 2) {
		_usage();
		return -1;
	} else if (strncmp(argv[1], "fail", 4) == 0) {
		if (!argv[2]) {
			puts("[!] input is not vaild");
			return -1;
		}
		us_fail_node = strtol(argv[2], NULL, 10);
		if(us_fail_node > SK_N_S) {
			puts("[!] input is not vaild");
			return -1;
		}
		return 0;
	} else {
		printf("[!] unknown command: %s\n", argv[1]);
		return -1;
	}

	return 0;
}

void *skin_node_sim_thread(ATTR_UNUSED void *arg)
{
	uint8_t i, j, b;

	/*
	 * PAY ATTENTION:
	 * If you run this module without MODULE IHBCAN or extra delay,
	 * this loop becomes a CPU killer like
	 */
	while (true) {

		/* Fill the struct ihb_structs with fake data */
		for(i = 0; i < SK_N_S; i++) {

			/* Address... */
			sk[i].address = i;

			/* Force a skin node in failure state */
			if(i == us_fail_node)
				sk[i].expired = true;

			/* Tactails... */
			for(j = 0; j < SK_T_S; j++) {
				random_bytes(&b, 1);
				sk[i].data[j] = b;
			}
		}

		/* Printing to stdout breaks the timings */
		if (ENABLE_DEBUG) {
			for(i = 0; i < SK_N_S; i++) {
				DEBUG("[0x%04x] Tactiles: ", sk[i].address);
				for(j = 0; j < SK_T_S; j++)
					DEBUG(" %d=%02x", j, sk[i].data[j]);
				puts(" ");
			}
		}

		/* To check the isotp_ready flag the struct CAN must be valid */
		if(IHB->can_isotp_ready)
#ifdef MODULE_IHBCAN
			ihb_isotp_send_chunks(sk, buffer_len);
#else
			xtimer_usleep(WAIT_1000ms);
#endif
		else
			thread_sleep();

		/*
		 * You can add a delay if you needs it
		 * xtimer_usleep(WAIT_20ms);
		 */
	}
}

int ihb_init_netsimulator(struct ihb_ctx *ihb_ctx)
{
	sk = xcalloc(SK_N_S, data_bs);
	IHB = ihb_ctx;

	IHB->ihb_info.skin_nodes = SK_N_S;
	IHB->ihb_info.skin_tactails = SK_T_S;

	printf("[*] Skin node simulator: Nodes=%u Sensors per node=%u\n",
			SK_N_S, SK_T_S);

	/* Initialize speudo_random*/
	random_init(13);

	return thread_create(skin_sim_stack,
				sizeof(skin_sim_stack),
				THREAD_PRIORITY_MAIN - 1,
				THREAD_CREATE_SLEEPING,
				skin_node_sim_thread,
				NULL, SK_THREAD_HELP);
}

void ihb_skin_module_info(void)
{
	printf("Tactile sensors for node=%u\nSkin nodes=%u\n",
		SK_T_S,
		SK_N_S);
}
