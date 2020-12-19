#ifndef CAN_H
#define CAN_H

#include <stdio.h>
#include <stdlib.h>

#include "../ihb.h"

/* MCU can have one or more CAN controllers, use first (0) */
#define CAN_C (0)
#if defined CAN_DLL_NUMOF && CAN_DLL_NUMOF == 0
#error "NO CAN controller avaible"
#endif

/* Timeout for ISO TP transmissions */
#ifndef ISOTP_TIMEOUT_DEF
#define ISOTP_TIMEOUT_DEF (3u)
#endif

#ifndef IHBTOLL_FRAME_ID
#define IHBTOLL_FRAME_ID 0x100
#endif

/* Maximum length for MCU id string */
#define MAX_MCU_ID_LEN (16)
#if defined CPUID_LEN && MAX_MCU_ID_LEN <= CPUID_LEN
#error "CPUID_LEN > MAX_CPUID_LEN"
#endif

/* Maximum length of CAN driver name */
#define MAX_NAME_LEN (16)

typedef enum {
	IDLE = 0x0,
	NOTIFY,
	BACKUP,
	ACTIVE,
	TOFIX,
	ERR
} fsm_state_t;

typedef enum {
	WAKEUP = 0x0,
	MASTER,
	SLAVE,
	RUNT_FIX,
	FIXED,
	FAIL
} fsm_event_t;

typedef struct state_transition_table {
	fsm_state_t state;
	fsm_event_t trans;
	fsm_state_t nextstate;
} fsm_table_t;

/* FILE: ihb-can/main.c */

/*
 * ihb_can_module_info() - print can submodule info
 */
void ihb_can_module_info(void);

/*
 * ihb_init_can() - inizialize the CAN module.
 *
 * @ihb_ctx: ihb contex
 *
 * In case of success, it populates the CANs entries of @ihb_ctx
 * Return pid of can handler thread on a success, num < 0 otherwise.
 */
int ihb_init_can(struct ihb_ctx *ihb_ctx);

/* FILE: ihb-can/sender.c */

/*
 * ihb_isotp_close() - close the isotp connnection and sets can_isotp_ready to
 * 		       false.
 *
 * It returns 0 in case of success.
 */
int ihb_isotp_close(void);

/*
 * ihb_isotp_init() - create a isotp socket and bind it
 *
 * @can_num: peripheral identifier of the MCU's CAN controller
 * @conn_timeout: timeout for ISO TP transmissions
 * @ready: pointer of can_isotp_ready
 *
 * It returns 0 in case of success.
 */
int ihb_isotp_init(uint8_t can_num, uint8_t conn_timeout, bool *ready);

/*
 * ihb_isotp_send_validate() - validate the chunks before to sed .
 *
 * @data: pointer of the data which have to send
 * @lenght: data block size
 *
 * If ihb have to send the same data types, the inputs will be verified only the
 * first time because the data typess never changes.
 *
 * This function reduce number of check did on ihb_isotp_send_chunks
 *
 * Return 0 if all is well, negative number otherwise.
 */
int ihb_isotp_send_validate(const void *data, const size_t length);

/*
 * ihb_isotp_send_chunks() - send chunks of data by iso-tp protocol.
 *
 * @data: pointer of the data which have to send
 * @lenght: data block size
 *
 * It returns 0 in case of success and can shutdown the isotp connection if it
 * goes in timeout (set false the flag can_isotp_ready).
 */
int ihb_isotp_send_chunks(const void *data, const size_t length);

/* FILE: ihb-can/fsm.c */

/*
 * state_init() - initialize the finite state machine to IDLE
 */
void state_init(void);

/*
 * state_event() - skip towards next state in relation to current state and event
 *
 * @evnt: event type
 */
void state_event(fsm_event_t evnt);

/*
 * state_is() - test the current state
 *
 * @in_states: the state which to have check
 *
 * Return true if current state  is @@in_states, false otherwise
 */
bool state_is(fsm_state_t in_states);

/*
 * state_print() - returns into about current state
 *
 * Return a pointer which contains the name of current state.
 */
char *state_print(void);

#endif
