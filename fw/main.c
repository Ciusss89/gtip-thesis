/**
 * Author:	Giuseppe Tipaldi
 * Created:	2020
 */
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* API's RIOT-OS */
#include "shell.h"
#include "board.h"
#include "thread.h"

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

static struct ihb_ctx IHB = {0};

static int ihb_struct_list(ATTR_UNUSED int argc, ATTR_UNUSED char **argv)
{
	puts("[*] IHB info");

#ifdef MODULE_IHBCAN
	if(IHB.can) {
		ihb_can_module_info(IHB.can);
		printf("- PIDs:\n\tnotify=%d\n", IHB.pid_can_handler);
	} else {
		puts("[!] BUG: struct ihb_can_ctx never should be null");
	}
#endif

#ifdef MODULE_IHBNETSIM
	if(IHB.sk_nodes) {
		ihb_skin_module_info(IHB.sk_nodes);
		printf("- PIDs:\n\tNetSkinSimulator=%d\n", *IHB.pid_ihbnetsim);
	} else {
		puts("[!] BUG: struct sk_nodes never should be null");
	}
#endif
	return 0;
}

/* Add custom tool to system shell */
static const shell_command_t shell_commands[] = {
#ifdef MODULE_IHBNETSIM
	{ "skin", SK_USERSPACE_HELP, skin_node_handler},
#endif
	{ "ihb", "ihb data info", ihb_struct_list},
	{ NULL, NULL, NULL }
};
static char line_buf[SHELL_DEFAULT_BUFSIZE];

static void ihb_info_init(void)
{
	strncpy(IHB.ihb_info.mcu_arch, RIOT_MCU, strlen(RIOT_MCU) + 1);
	strncpy(IHB.ihb_info.mcu_board, RIOT_BOARD, strlen(RIOT_BOARD) + 1);
	strncpy(IHB.ihb_info.riotos_ver, RIOT_VERSION, strlen(RIOT_VERSION) + 1);
	strncpy(IHB.ihb_info.ihb_fw_rev, IHB_FW_VER, strlen(IHB_FW_VER) + 1);
}

/**
 * @ihb_init: start the ihb routine
 *
 * Return 0 if all is well, a negative number otherwise.
 */
static int ihb_init(void)
{
	kernel_pid_t pid_data_gen = -1;

	ihb_info_init();

#ifdef MODULE_UPTIME
	puts("[*] MODULE_UPTIME");
	uptime_thread_start();
#endif

#ifdef MODULE_IHBNETSIM
	IHB.sk_nodes = NULL;
	IHB.pid_ihbnetsim = NULL;
	puts("[*] MODULE_IHBNETSIM");

	pid_data_gen = thread_create(skin_sim_stack,
			             sizeof(skin_sim_stack),
				     THREAD_PRIORITY_MAIN - 1,
				     THREAD_CREATE_SLEEPING,
				     skin_node_sim_thread,
				     (void *)&IHB, SK_THREAD_HELP);
	if(pid_data_gen < KERNEL_PID_UNDEF) {
		puts("[!] cannot start netskin thread");
		return -1;
	}

	IHB.pid_ihbnetsim = &pid_data_gen;
	/* Force thread to start immediately */
	thread_wakeup(pid_data_gen);
	if (!IHB.pid_ihbnetsim || !IHB.sk_nodes)
		return -1;
#endif

#ifdef MODULE_IHBCAN
	IHB.can = NULL;
	puts("[*] MODULE_IHBCAN");

	/*
	 * To work IHBCAN needs the data which must be sent to host by isotp,
	 * pid_data_gen consists in the thread which will be turn on by this
	 * module to acquire the data to send.
	 */
	if(pid_data_gen < KERNEL_PID_UNDEF)
		return -1;

	IHB.pid_can_handler = ihb_can_init(&IHB, pid_data_gen);
	if(!IHB.can || IHB.pid_can_handler < KERNEL_PID_UNDEF)
		return -1;
#endif

	/* Silent the gcc warning */
	(void) pid_data_gen;

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
