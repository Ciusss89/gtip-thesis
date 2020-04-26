/**
 * Author:	Giuseppe Tipaldi
 * Created:	2020
 *
 */
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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

#ifdef MODULE_IHBTOOLS
#include "ihb-tools/tools.h"
#else
#error "need ihb-tools lib"
#endif

#include "ihb.h"

static struct ihb_structs IHB;

int ihb_struct_list(__attribute__((unused)) int argc, __attribute__((unused)) char **argv)
{
	puts("[*] IHB info");

#ifdef MODULE_IHBCAN
	const struct ihb_can_perph *can = IHB.can;
	if(IHB.can) {
		printf("- CAN: struct ihb_can_perph address=%p, size=%ubytes",
			(void *)IHB.can,
			sizeof(struct ihb_can_perph));

		printf("\n\tdev=%d\n\tname=%s\n\tmcu_id=%s\n\tframe_id=%#x\n\trole=%s\n\tnotify=%s\n",
			can->id,
			can->name,
			can->controller_uid,
			can->frame_id,
			can->master ? "master" : "idle",
			can->status_notify_node ? "is running" : "no");

		printf("- PIDs:\n\tnotify=%d\n", *IHB.pid_notify_node);

	} else {
		puts("[!] BUG: struct can never should be null");
	}
#endif
#ifdef MODULE_IHBNETSIM
	if(IHB.sk_nodes) {
		printf("- SKIN: struct skin_nodes address=%p, size=%ubytes",
			(void *)IHB.sk_nodes,
			sizeof(struct skin_node));

		printf("\n\tTactile sensors for node=%u\n\tSkin nodes=%u\n",
			SK_T_S,
			SK_N_S);

		printf("- PIDs:\n\tNetSkinSimulator=%d\n", *IHB.pid_ihbnetsim);

	} else {
		puts("[!] BUG: struct sk_nodes never should be null");
	}
#endif

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
	memset(&IHB, 0, sizeof(struct ihb_structs));
	IHB.ihb_info = xmalloc(sizeof(struct ihb_node_info));
#ifdef MODULE_IHBCAN
	IHB.ihb_info->isotp_timeo = ISOTP_TIMEOUT_DEF;
#endif
	strncpy(IHB.ihb_info->mcu_arch, RIOT_MCU, strlen(RIOT_MCU) + 1);
	IHB.ihb_info->mcu_arch[strlen(RIOT_MCU)] = '\0';

	strncpy(IHB.ihb_info->mcu_board, RIOT_BOARD, strlen(RIOT_BOARD) + 1);
	IHB.ihb_info->mcu_board[strlen(RIOT_BOARD)] = '\0';

	strncpy(IHB.ihb_info->riotos_ver, RIOT_VERSION, strlen(RIOT_VERSION) + 1);
	IHB.ihb_info->riotos_ver[strlen(RIOT_VERSION)] = '\0';

	strncpy(IHB.ihb_info->ihb_fw_rev, IHB_FW_VER, strlen(IHB_FW_VER) + 1);
	IHB.ihb_info->ihb_fw_rev[strlen(IHB_FW_VER)] = '\0';

#ifdef MODULE_UPTIME
	puts("[*] MODULE_UPTIME");
	uptime_thread_start();
#endif

#ifdef MODULE_IHBNETSIM
	IHB.sk_nodes = NULL;
	puts("[*] MODULE_IHBNETSIM");
	kernel_pid_t pid_ihbnetsim = thread_create(skin_sim_stack,
					  sizeof(skin_sim_stack),
					  THREAD_PRIORITY_MAIN - 2,
					  THREAD_CREATE_WOUT_YIELD,
					  _skin_node_sim_thread,
					  (void *)&IHB, SK_THREAD_HELP);

	if(pid_ihbnetsim < KERNEL_PID_UNDEF)
		puts("[!] cannot start the skin simulator thread");

	IHB.pid_ihbnetsim = &pid_ihbnetsim;
#endif

#ifdef MODULE_IHBCAN
	IHB.can = NULL;
	puts("[*] MODULE_IHBCAN");
	return _can_init(&IHB);
#endif

	return 0;
}

int main(void)
{
	printf("RIOT-OS=%s App=%s Board=%s MCU=%s, IHBv=%s\n\r",
		RIOT_VERSION,
		RIOT_APPLICATION,
		RIOT_BOARD,
		RIOT_MCU,
		IHB_FW_VER);

	if(ihb_init() < 0)
		puts("[!] IHB: init of system has falied");

	shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

	 /* should be never reached */
	return 0;
}
