#ifndef CAN_H
#define CAN_H

#include <stdio.h>
#include <stdlib.h>
#include "ihb.h"

void *_thread_send2host(void *IHB);
int _can_init(struct ihb_structs *IHB);
int _ihb_can_handler(int argc, char **argv);
#endif
