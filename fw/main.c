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
#endif

#ifdef MODULE_IHBCAN
#include "ihb-can/ihbcan.h"
static struct ihb_can_perph can_0;
#endif

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

int main(void)
{
	int r = -1;

#ifdef MODULE_UPTIME
	uptime_thread_start();
#endif

#ifdef MODULE_IHBNETSIM
	int pid_ihbnetsim = thread_create(skin_sim_stack,
					  sizeof(skin_sim_stack),
					  THREAD_PRIORITY_MAIN - 2,
					  THREAD_CREATE_WOUT_YIELD,
					  _skin_node_sim_thread,
					  NULL, SK_THREAD_HELP);

	if(pid_ihbnetsim < KERNEL_PID_UNDEF)
		puts("cannot create skin simulator thread");
#endif

#ifdef MODULE_IHBCAN
	r = _can_init(&can_0);
	if (r != 0)
		puts("_can_init: has failed\n");
#endif

	printf("RIOT-OS, MCU=%s Board=%s\n\r", RIOT_MCU, RIOT_BOARD);
	shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

	 /* should be never reached */
	return r;
}
