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
#define SK_USERSPACE_HELP "ihb-netsim modules info"
#include "ihb-netsim/skin.h"
#endif

#ifdef MODULE_IHBCAN
#define CAN_USERSPACE_HELP "ihb-can modules info"
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
	ihb_can_module_info();
	printf("PID: can submodule=%d\n", IHB.pid_can_handler);
#endif

#ifdef MODULE_IHBNETSIM
	ihb_skin_module_info();
	printf("PID: netsim submodule=%d\n", IHB.pid_skin_handler);
#endif
	return 0;
}

/* Add custom tool to system shell */
static const shell_command_t shell_commands[] = {
#ifdef MODULE_IHBNETSIM
	{ "skin", SK_USERSPACE_HELP, skin_node_handler},
#endif
	{ "ihb", CAN_USERSPACE_HELP, ihb_struct_list},
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
static int ihb_modules_init(void)
{
	ihb_info_init();

#ifdef MODULE_IHBNETSIM
	IHB.pid_skin_handler = ihb_init_netsimulator(&IHB);
	if(IHB.pid_skin_handler < KERNEL_PID_UNDEF)
		return -1;

	thread_wakeup(IHB.pid_skin_handler);
	puts("[*] MODULE_IHBNETSIM [OK]");
#endif

#ifdef MODULE_IHBCAN
	IHB.pid_can_handler = ihb_init_can(&IHB);
	if(IHB.pid_can_handler < KERNEL_PID_UNDEF)
		return -1;

	thread_wakeup(IHB.pid_can_handler);
	puts("[*] MODULE_IHBCAN [OK]");
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

	if(ihb_modules_init() < 0)
		puts("[!] IHB: init of system has falied");

	shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

	 /* should be never reached */
	return 0;
}
