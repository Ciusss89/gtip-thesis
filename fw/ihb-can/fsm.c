/**
 * Author:	Giuseppe Tipaldi
 * Created:	2020
 */
#include <stdio.h>
#include <stdbool.h>

#include "can.h"

static fsm_state_t can_state;

static fsm_table_t transition[] = {
	{IDLE, WAKEUP, NOTIFY},
	{NOTIFY, MASTER, ACTIVE},
	{NOTIFY, SLAVE, BACKUP},
	{NOTIFY, RUNT_FIX, TOFIX},
	{BACKUP, MASTER, ACTIVE},
	{TOFIX, FIXED, NOTIFY},
	{ACTIVE, FAIL, ERR}
};

void state_event(fsm_event_t evnt)
{
	for(uint8_t i = 0; i < sizeof(transition); ++i) {
		if((transition[i].state == can_state) &&
		   (transition[i].trans == evnt)) {
			can_state = transition[i].nextstate;
			return;
		}
	}

	can_state = ERR;
}

bool state_is(fsm_state_t in_states)
{
	if (can_state == in_states)
		return true;
	else
		return false;
}

void state_init(void)
{
	can_state = IDLE;
}

void state_print(const char *ctx)
{
	switch (can_state)
	{
		case IDLE:
			printf("[*] IHB %s, state = IDLE\n", ctx);
			break;
		case NOTIFY:
			printf("[*] IHB %s, state = NOTIFY\n", ctx);
			break;
		case BACKUP:
			printf("[*] IHB %s, state = BACKUP\n", ctx);
			break;
		case ACTIVE:
			printf("[*] IHB %s, state = ACTIVE\n", ctx);
			break;
		case TOFIX:
			printf("[*] IHB %s, state = TOFIX\n", ctx);
			break;
		case ERR:
			printf("[*] IHB %s, state = ERROR\n", ctx);
			break;
		default:
			printf("[*] IHB %s state = ??\n", ctx);
			break;
	}
}
