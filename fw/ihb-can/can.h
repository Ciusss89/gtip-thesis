#ifndef CAN_H
#define CAN_H

#include <stdio.h>
#include <stdlib.h>
#include "ihb.h"

/*
 * IPC messages:
 */
#define CAN_MSG_START_ISOTP	0x400
#define CAN_MSG_SEND_ISOTP	0x401
#define CAN_MSG_CLOSE_ISOTP	0x402

void *_thread_send2host(void *IHB);
int _can_init(struct ihb_structs *IHB);
int _ihb_can_handler(int argc, char **argv);
#endif
