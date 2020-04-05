/**
 * Author:	Giuseppe Tipaldi
 * Created:	2020
 *
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

/* API's RIOT-OS */
#include "shell.h"
#include "board.h"
#include "thread.h"

/* custom usermodules */
#ifdef MODULE_UPTIME
#include "uptime/uptime.h"
#endif

#ifdef MODULE_IHBNETSIM
#include "ihb-netsim/skin.h"
char skin_sim_stack[THREAD_STACKSIZE_MEDIUM];
static struct skin_node skin_nodes[SK_N_S];
#endif

#ifdef MODULE_IHBCAN
#include "ihb-can/can.h"
static struct ihb_can_perph *can_0 = NULL;
#endif

/* Add custom tool to system shell */
static const shell_command_t shell_commands[] = {
#ifdef MODULE_UPTIME
	{ "uptime", UPTIME_THREAD_HELP, _uptime_handler},
#endif
#ifdef MODULE_IHBCAN
	{ "ihbcan", IHB_THREAD_HELP, _ihb_can_handler},
#endif
	{ NULL, NULL, NULL }
};
static char line_buf[SHELL_DEFAULT_BUFSIZE];

/**
 * @ihb_init: start the ihb routine
 *
 * Return 0 if all is well, a negative number otherwise.
 */
static int ihb_init(void)
{
	int r;

#ifdef MODULE_UPTIME
	puts("[*] MODULE_UPTIME");
	uptime_thread_start();
#endif

#ifdef MODULE_IHBNETSIM
	puts("[*] MODULE_IHBNETSIM");
	int pid_ihbnetsim = thread_create(skin_sim_stack,
					  sizeof(skin_sim_stack),
					  THREAD_PRIORITY_MAIN - 2,
					  THREAD_CREATE_WOUT_YIELD,
					  _skin_node_sim_thread,
					  (void *)&skin_nodes, SK_THREAD_HELP);

	if(pid_ihbnetsim < KERNEL_PID_UNDEF)
		puts("[!] cannot start the skin simulator thread");
#endif

#ifdef MODULE_IHBCAN
	puts("[*] MODULE_IHBCAN");
	/*
	 * TODO!
	 * Issue: IHBCAN needs IHBNETSIM
	 *
	 * I don't like this, find a way to make this module indipend from
	 * the module IHBNETSIM
	 */
	r = _can_init(&can_0, skin_nodes);
	if (r != 0)
		puts("[!] init of the CAN submodule has failed");
#endif

	return r;
}

int main(void)
{
	if(ihb_init() < 0)
		puts("[!] IHB: init of system has falied");


	printf("RIOT-OS, MCU=%s Board=%s\n\r", RIOT_MCU, RIOT_BOARD);
	shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

	 /* should be never reached */
	return 0;
}
