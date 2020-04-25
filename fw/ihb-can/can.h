#ifndef CAN_H
#define CAN_H

#include <stdio.h>
#include <stdlib.h>
#include "ihb.h"

void *_thread_send2host(void *IHB);
int _can_init(struct ihb_structs *IHB);
int _ihb_can_handler(int argc, char **argv);
void ihb_isotp_send_chunks(const void *in_data, size_t data_bs, size_t nmemb);
#endif
