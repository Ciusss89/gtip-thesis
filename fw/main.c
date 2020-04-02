#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include "shell.h"
#include "board.h"
#include "periph/gpio.h"
#include "thread.h"

#ifdef MODULE_UPTIME
#include "uptime/uptime.h"
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
