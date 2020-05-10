#ifndef CAN_H
#define CAN_H

#include <stdio.h>
#include <stdlib.h>

#include "thread.h"

#define IHB_USERSPACE_HELP "ihb-can userspace tool"
#define IHB_THREAD_HELP "ihb-can driver"

/* Maximum length of CAN driver name */
#define CAN_NAME_LEN (16 + 1)

/* Maximum length for MCU id string */
#define MAX_MCU_ID_LEN (16u)
#if defined CPUID_LEN && MAX_MCU_ID_LEN <= CPUID_LEN
#error "CPUID_LEN > MAX_CPUID_LEN"
#endif

/* MCU can have one or more CAN controllers, use first (0) */
#define CAN_C (0)
#if defined CAN_DLL_NUMOF && CAN_DLL_NUMOF == 0
#error "NO CAN controller avaible"
#endif

/* Timeout for ISO TP transmissions */
#ifndef ISOTP_TIMEOUT_DEF
#define ISOTP_TIMEOUT_DEF (3u)
#endif

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

/**
 * struc ihb_can_ctx - ihb can context struct contains data which are used by
 * 		       can submodule.
 *
 * @mcu_controller_uid: MCU's unique ID
 * @can_drv_name: the name of the can controller
 * @status_notify_node: if true the IHB node is announcing itself on CAN bus
 * @can_frame_id: can frame identifier, it's obtainded by the MCU unique ID
 * @can_isotp_ready: if true the IHB can send the isotp chunks to host
 * @can_perh_id: peripheral identifier of the MCU's CAN controller
 * @master: true when the IHB node is master
 */
struct ihb_can_ctx {
	char mcu_controller_uid[MAX_MCU_ID_LEN * 2 + 1];
	char can_drv_name[CAN_NAME_LEN];
	bool status_notify_node;
	uint8_t can_frame_id;
	bool can_isotp_ready;
	uint8_t can_perh_id;
	bool master;
};

/* FILE: ihb-can/main.c */

/*
 * @ihb_can_module_info: print ihb_can_ctx struct contens
 *
 * @ctx: pointer of struct ihb_can_ctx
 */
void ihb_can_module_info(struct ihb_can_ctx *ctx);

/*
 * @ihb_can_init: inizialize the CAN module.
 *
 * @ctx: pointer of struct ihb_ctx
 * @_data_source: pid of process which generates payload
 *
 * In case of success it adds the ihb_can_ctx struct pointer to ihb_ctx.
 *
 * Return pid of can handler thread on a success, num < 0 otherwise.
 */
int ihb_can_init(void *ctx, kernel_pid_t _data_source);

/* FILE: ihb-can/sender.c */

/*
 * @ihb_isotp_close: close the isotp connnection and sets can_isotp_ready to
 *                   false.
 *
 * It returns 0 in case of success.
 */
int ihb_isotp_close(void);

/*
 * @ihb_isotp_init: create a isotp socket and bind it
 *
 * @can_num: peripheral identifier of the MCU's CAN controller
 * @conn_timeout: timeout for ISO TP transmissions
 * @ready: pointer of can_isotp_ready
 *
 * It returns 0 in case of success.
 */
int ihb_isotp_init(uint8_t can_num, uint8_t conn_timeout, bool *ready);

/*
 * @ihb_isotp_send_chunks: send chunks of data by iso-tp protocol. Function makes
 *                         a copy of in_data and sends it.
 *
 * @in_data: pinter of the data which have to send
 * @data_bs: data block size
 * @count: count of data_bs which have to send
 *
 * It returns 0 in case of success and can shutdown the isotp connection if it
 * goes in timeout (set false the flag can_isotp_ready).
 */
int ihb_isotp_send_chunks(const void *in_data, size_t data_bs, size_t count);

/* FILE: ihb-can/fsm.c */

/*
 * @state_init: initialize the finite state machine to IDLE
 */
void state_init(void);

/*
 * @state_event: skip towards next state in relation to current state and event
 *
 * @evnt: event type
 */
void state_event(fsm_event_t evnt);

/*
 * @state_is: test the current state
 *
 * @in_states: the state which to have check
 *
 * Return true if current state  is @@in_states, false otherwise
 */
bool state_is(fsm_state_t in_states);

/*
 * @state_print: print to stdout the current state
 *
 * @ctx: message to appen.
 */
void state_print(const char *ctx);
#endif
