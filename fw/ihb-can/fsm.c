/**
 * Author:	Giuseppe Tipaldi
 * Created:	2020
 */
#include <stdio.h>
#include <stdbool.h>

#define ENABLE_DEBUG    (0)
#include "debug.h"

#include "can.h"

static fsm_state_t can_state;

static fsm_table_t transition[] = {
	{IDLE, WAKEUP, NOTIFY},
	{NOTIFY, MASTER, ACTIVE},
	{NOTIFY, SLAVE, BACKUP},
	{NOTIFY, RUNT_FIX, TOFIX},
	{BACKUP, MASTER, ACTIVE},
	{BACKUP, SLAVE, BACKUP},
	{TOFIX, FIXED, NOTIFY},
	{ACTIVE, FAIL, ERR}
};

void state_event(fsm_event_t evnt)
{
	for(uint8_t i = 0; i < sizeof(transition); ++i) {
		if((transition[i].state == can_state) &&
		   (transition[i].trans == evnt)) {
			can_state = transition[i].nextstate;
			DEBUG("[#] IHB state-next-->%s\n",state_print());
			return;
		}
	}

	/* This never should happen */
	puts("[!] IHB transition not allowed, force error status");
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

char *state_print(void)
{
	switch (can_state)
	{
		case IDLE:
			return "IDLE";
		case NOTIFY:
			return "NOTIFY";
		case BACKUP:
			return "BACKUP";
		case ACTIVE:
			return "ACTIVE";
		case TOFIX:
			return "TOFIX";
		case ERR:
			return "ERROR";
		default:
			return "NOT DEFINED";
	}

	return NULL;
}
