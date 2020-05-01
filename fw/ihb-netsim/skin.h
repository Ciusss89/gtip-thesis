#ifndef CAN_NETSIM_H
#define CAN_NETSIM_H

#include <stdlib.h>
#include <stdio.h>

int skin_node_handler(int argc, char **argv);
void *skin_node_sim_thread(void *in);
#endif
