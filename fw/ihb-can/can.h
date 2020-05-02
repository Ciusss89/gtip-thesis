#ifndef CAN_H
#define CAN_H

#include <stdio.h>
#include <stdlib.h>
#include "ihb.h"

/* ihb-can/main.c */
int ihb_can_handler(int argc, char **argv);
void ihb_can_init(struct ihb_structs *IHB, kernel_pid_t _data_source);

/* ihb-can/sender.c */
int ihb_isotp_close(void);
int ihb_isotp_init(uint8_t can_num, uint8_t conn_timeout, bool *ready);
int ihb_isotp_send_chunks(const void *in_data, size_t data_bs, size_t nmemb);

#endif
