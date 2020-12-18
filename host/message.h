#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdio.h>

/* ASCII message "IHB-ID" : */
const uint8_t IHBMAGIC[6] = {0x49, 0x48, 0x42, 0x2D, 0x49, 0x44};

/* ASCII message "IHB-WKUP" : switch the node in notify state */
const char *msg_wkup = "4948422D574B5550";
/* ASCII message "IHB-MSTR" : switch the node in action state */
const char *msg_mstr = "4948422D4D535452";
/* ASCII message "IHB-BCKP" : switch the node in backup state */
const char *msg_bckp = "4948422D42434B50";
/* ASCII message "IHB-" : receive new address by ihbtool, runtime fix */
const char *msg_add_fix = "4948422D";
#endif
