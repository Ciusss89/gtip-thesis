#include <stdlib.h>

#include "xtimer.h"
#include "timex.h"
#include "thread.h"

#include "uptime.h"

#define ENABLE_DEBUG    (0)
#include "debug.h"

#define UPTIME_THREAD_INFO "count elapsed time"

char uptime_stack[THREAD_STACKSIZE_SMALL];

static int8_t pid_uptime = -1;

static uint8_t s = 0;
static uint8_t m = 0;
static uint8_t h = 0;
static uint16_t d = 0;

static void *uptime_thread(void *arg)
{
	(void)arg;

	while(1) {
		xtimer_sleep(1);
		s++;

		if(s == 60) {
			m++;
			s = 0;
		}

		if (m == 60) {
			h++;
			m = 0;
		}

		if (h == 24) {
			d++;
			h = 0;
		}
		DEBUG("[*] uptime: day=%u hour=%u minute=%u second=%u\n", d, h, m, s);
	};
	
	/* should be never reached */
	return NULL;
}

int _uptime_handler(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	if (pid_uptime > 0)
		printf("uptime: day=%u hour=%u minute=%u second=%u\n", d, h, m, s);
	else {
		puts("uptime: thread isn't running.\n");
		return -1;
	}

	return 0;
}

int uptime_thread_start(void)
{
	pid_uptime = thread_create(uptime_stack, sizeof(uptime_stack),
				   THREAD_PRIORITY_MAIN + 1,
				   THREAD_CREATE_WOUT_YIELD,
				   uptime_thread, NULL, UPTIME_THREAD_INFO);

	if (pid_uptime <  KERNEL_PID_UNDEF)
		return -1;

return 0;
}
