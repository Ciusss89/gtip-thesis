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
#endif

#ifdef MODULE_IHBCAN
#include "ihb-can/can.h"
#endif

#include "ihb.h"

static struct ihb_structs IHB;

int ihb_struct_list(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	puts("[*] IHB info");

#ifdef MODULE_IHBCAN
	const struct ihb_can_perph *can = IHB.can;
	if(IHB.can) {
		printf("- CAN: struct ihb_can_perph address=%p, size=%ubytes",
			IHB.can,
			sizeof(struct ihb_can_perph));

		printf("\n\tdev=%d\n\tname=%s\n\tmcu_id=%s\n\tframe_id=%#x\n\trole=%s\n\tnotify=%s\n",
			can->id,
			can->name,
			can->controller_uid,
			can->frame_id,
			can->master ? "master" : "idle",
			can->status_notify_node ? "is running" : "no");

		printf("- PIDs:\n\tnotify=%d\n\tsend2host=%d\n",
			*IHB.pid_notify_node,
			*IHB.pid_send2host);

	} else {
		puts("[!] BUG: this struct never should be null");
	}
#endif
#ifdef MODULE_IHBNETSIM
	if(IHB.can) {
		printf("- SKIN: struct skin_nodes address=%p, size=%ubytes",
			IHB.sk_nodes,
			sizeof(struct skin_node));

		printf("\n\tTactile sensors for node=%u\n\tSkin nodes=%u\n",
			SK_T_S,
			SK_N_S);
	} else {
		puts("[!] BUG: this struct never should be null");
	}
#endif

	/* !TODO add fw versioning and others info */

	return 0;
}

/* Add custom tool to system shell */
static const shell_command_t shell_commands[] = {
#ifdef MODULE_UPTIME
	{ "uptime", UPTIME_THREAD_HELP, _uptime_handler},
#endif
#ifdef MODULE_IHBCAN
	{ "ihbcan", IHB_THREAD_HELP, _ihb_can_handler},
#endif
	{ "ihb", "ihb data info", ihb_struct_list},
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
	IHB.sk_nodes = NULL;
	puts("[*] MODULE_IHBNETSIM");
	int pid_ihbnetsim = thread_create(skin_sim_stack,
					  sizeof(skin_sim_stack),
					  THREAD_PRIORITY_MAIN - 2,
					  THREAD_CREATE_WOUT_YIELD,
					  _skin_node_sim_thread,
					  (void *)&IHB, SK_THREAD_HELP);

	if(pid_ihbnetsim < KERNEL_PID_UNDEF)
		puts("[!] cannot start the skin simulator thread");
#endif

#ifdef MODULE_IHBCAN
	IHB.can = NULL;
	puts("[*] MODULE_IHBCAN");
	r = _can_init(&IHB);
	if (r != 0)
		puts("[!] init of the CAN submodule has failed");
#endif

	return r;
}

int main(void)
{
	printf("RIOT-OS=%s App=%s Board=%s MCU=%s\n\r", RIOT_VERSION,
							RIOT_APPLICATION,
							RIOT_BOARD,
							RIOT_MCU);

	if(ihb_init() < 0)
		puts("[!] IHB: init of system has falied");

	shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

	 /* should be never reached */
	return 0;
}
