#ifndef UPTIME_H
#define UPTIME_H

#include <stdio.h>

#define UPTIME_THREAD_HELP "uptime - Tell how long the system has been running."

int uptime_thread_start(void);
int _uptime_handler(int argc, char **argv);
#endif
