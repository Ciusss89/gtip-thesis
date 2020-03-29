#ifndef CAN_H
#define CAN_H

#include <stdio.h>
#include <stdlib.h>

#include "cpu_conf.h"
#include "periph/cpuid.h"

#define IHB_THREAD_HELP "ihb - can submodule, development branch"

#define WAIT_1000ms	(1000LU * US_PER_MS)	/* delay of 1 s */
#define RCV_TIMEOUT	(2000 * US_PER_MS)	/* socket rcv timeout */

/*
 * MCU can have one or more CAN controllers, by default I use the
 * CAN controlller 0
 */
#define CAN_C (0)

/* Maximum length of CAN name */
#define CAN_NAME_LEN (16 + 1)

#define MAX_CPUID_LEN (16)

struct ihb_can_perph {
	char controller_uid[MAX_CPUID_LEN * 2 + 1];
	char name[CAN_NAME_LEN];

	/* Device Identifier Number: number of can the controllers of the mcu */
	uint8_t id;

	/* CAN frame ID */
	uint8_t frame_id;

	bool status_notify_node;
};

enum ihb_states
{
	IHB_IDLE,
	IHB_NOTIFY,
	IHB_READY,
	IHB_SCAN,
	IHB_BUSY,
	IHB_RISEVATE,
	IHB_RISERVATE1,
	IHN_RISERVATE2,
	IHB_RISERVATE3,
	IHB_RISERVATE4,
	IHB_ERR,
	IHB_MAX_STATES
};


void oom(void);
void *xmalloc(size_t size);
char *data2str(const unsigned char *data, size_t len);
uint8_t fletcher8(const unsigned char * data, size_t n);

int _can_init(struct ihb_can_perph *device);
int _ihb_can_handler(int argc, char **argv);
#endif
