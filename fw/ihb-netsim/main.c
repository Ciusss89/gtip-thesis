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

/* TAXEL ARRAY: ADC resolution (16 bit) * taxel amout (12) */
const size_t skin_adc_payload = sizeof(uint16_t) * SK_T_S;

/* SKIN MODULE BLOCK DATA SIZE */
const size_t skin_data_bs = sizeof(struct skin_node);

/* SKINS' ARRAY */
const size_t skins_buffer_len = SK_N_S * skin_data_bs;

uint8_t skin_fail_node = SKIN_MAX_COUNT;
static struct skin_node *sk = NULL;
struct ihb_ctx *IHB = NULL;

static void _usage(void)
{
	puts("SKIN patches simulator:");
	printf("skin fail <#node> \t - set as expired the #node (0-%d)\n", SK_N_S - 1);
	puts("skin start \t\t - wake up the thread");
	puts("skin stop \t\t - sleep the thread");
}

int skin_node_handler(int argc, char **argv)
{
	if (argc < 1 || argc > 3)
		goto fail;

	if (strncmp(argv[1], "start", 5) == 0) {
		if (!IHB)
			return -1;

		IHB->can_isotp_ready = true;
		thread_wakeup(IHB->pid_skin_handler);

	} else if (strncmp(argv[1], "stop", 4) == 0) {
		IHB->can_isotp_ready = false;

	} else if (strncmp(argv[1], "fail", 4) == 0) {
		if (!argv[2])
			goto fail;

		skin_fail_node = strtol(argv[2], NULL, 10);
		if (skin_fail_node >= SK_N_S || skin_fail_node >= SKIN_MAX_COUNT) {
			skin_fail_node = SKIN_MAX_COUNT;
			goto fail;
		}

	} else {
		goto fail;
	}

	return 0;
fail:
	_usage();
	return -1;
}

void *skin_node_sim_thread(ATTR_UNUSED void *arg)
{

	uint8_t i, j;

	/* Fake skin address ...*/
	for (i = 0; i < SK_N_S; i++)
		sk[i].address = i;


	while (true) {

		/* Fills all entries with fake data */
		for(i = 0; i < SK_N_S; i++) {
			/* Simulate a failure state */
			if (sk[i].address == skin_fail_node) {
				sk[i].expired = true;
				skin_fail_node = SKIN_MAX_COUNT;
			}

			/* spill fake data ... */
			random_bytes((uint8_t *)&sk[i].data[0], skin_adc_payload);
		}

		/* Printing to stdout breaks the timings */
		if (ENABLE_DEBUG) {
			for(i = 0; i < SK_N_S; i++) {
				DEBUG("[Addr=%#x] [Status=%s] Tactiles=[",
				      sk[i].address,
				      sk[i].expired ? "offline" : "online");
				for(j = 0; j < SK_T_S; j++)
					DEBUG(" %d=%#x", j, sk[i].data[j]);
				puts(" ]");
			}
		}

		if(IHB->can_isotp_ready)
#ifdef MODULE_IHBCAN
			ihb_isotp_send_chunks(sk, skins_buffer_len);
#else
			xtimer_usleep(WAIT_2000ms);
#endif
		else
			thread_sleep();

		/*
		 * You can add a delay if you need it
		 * xtimer_usleep(WAIT_20ms);
		 */
	}
}

int ihb_init_netsimulator(struct ihb_ctx *ihb_ctx)
{
	if (!ihb_ctx)
		return -1;

	sk = xcalloc(SK_N_S, skin_data_bs);
	IHB = ihb_ctx;

	IHB->ihb_info.skin_node_count = SK_N_S;
	IHB->ihb_info.skin_node_taxel = SK_T_S;

	printf("[*] Skin node simulator: Nodes=%u Sensors per node=%u\n",
			SK_N_S, SK_T_S);

	/* Initialize speudo_random*/
	random_init(13);

#ifdef MODULE_IHBCAN
	if (!ihb_isotp_send_validate(sk, skins_buffer_len))
		return -1;
#endif

	return thread_create(skin_sim_stack,
				sizeof(skin_sim_stack),
				THREAD_PRIORITY_MAIN - 1,
				THREAD_CREATE_SLEEPING,
				skin_node_sim_thread,
				NULL, SK_THREAD_HELP);
}

void ihb_skin_module_info(void)
{
	printf("Taxel sensors for node=%u\nSkin nodes=%u\n",
		SK_T_S,
		SK_N_S);
}
