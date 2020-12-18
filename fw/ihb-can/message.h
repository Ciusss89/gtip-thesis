#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdio.h>

/* ASCII message "IHB-ID" : ihb discovery message sent on CAN bus */
const unsigned char mgc[] = {0x49, 0x48, 0x42, 0x2D, 0x49, 0x44, 0};
/* ASCII message "IHB-" : receive new address by ihbtool, runtime fix */
unsigned char add_fix[8] = {0x49, 0x48, 0x42, 0x2D, 0x0, 0x0, 0x3D, 0x0};
/* ASCII message "IHB-WKUP" : switch the node in notify state */
const unsigned char wkup[] = {0x49, 0x48, 0x42, 0x2D, 0x57, 0x4B, 0x55, 0x50, 0};
/* ASCII message "IHB-MSTR" : switch the node in action state */
const unsigned char mstr[] = {0x49, 0x48, 0x42, 0x2D, 0x4D, 0x53, 0x54, 0x52, 0};
/* ASCII message "IHB-BCKP" : switch the node in backup state */
const unsigned char bckp[] = {0x49, 0x48, 0x42, 0x2D, 0x42, 0x43, 0x4B, 0x50, 0};
#endif
