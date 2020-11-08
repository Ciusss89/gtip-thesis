/**
 * Author:	Giuseppe Tipaldi
 * Created:	2020
 **/
#define _GNU_SOURCE

#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

/* RIOT APIs */
#include "can/conn/isotp.h"
#include "can/device.h"
#include "xtimer.h"

#define ENABLE_DEBUG    (0)
#include "debug.h"

#include "ihb-tools/tools.h"
#include "can.h"

/* Set the tx port for ISO TP transmissions */
#ifndef ISOTP_TX_PORT
#define ISOTP_TX_PORT 0x700
#endif

/* Set the rx port for ISO TP transmissions */
#ifndef ISOTP_RX_PORT
#define ISOTP_RX_PORT 0x708
#endif

static bool snd_chunk_fails = false;
static bool sck_ready = false;
static conn_can_isotp_t conn;
static uint8_t isotp_timeout;
static uint32_t first_fail;

/* It's isotp_ready of struct ihb_can_perph */
bool *ex_sck_ready = NULL;

static void isotp_sender_watchdog(void)
{
	uint32_t curr_fail = 0, diff = 0;

	if (!snd_chunk_fails) {
		/* Save the first fail  */
		snd_chunk_fails = true;
		first_fail = xtimer_now_usec() / US_PER_SEC;
	} else {
		/*
		 * if diff > isotp's timeout : the ihbtool will close the
		 * connection with IHB node.
		 */
		curr_fail = xtimer_now_usec() / US_PER_SEC;
		diff = curr_fail - first_fail;
	}

	DEBUG("[#] First Fail=%"PRIu32" - Curr Fail=%"PRIu32": Diff=%"PRIu32"\n",
	       first_fail, curr_fail, diff);

	if(diff > (uint32_t)isotp_timeout) {
		ihb_isotp_close();
		puts("[!] isotp shoutdown by watchdog");
	}
}

int ihb_isotp_init(uint8_t can_num, uint8_t conn_timeout, bool *ready)
{
	/* struct isotp_fc_options *fc_opt; */
	struct isotp_options isotp_opt;
	int r;

	if (sck_ready)
		return -EALREADY;

	ex_sck_ready = ready;

	isotp_timeout = conn_timeout;

	/* setup the socket */
	memset(&isotp_opt, 0, sizeof(isotp_opt));
	isotp_opt.tx_id = ISOTP_TX_PORT;
	isotp_opt.rx_id = ISOTP_RX_PORT;
	isotp_opt.txpad_content = 0xCE;

	/* Start the IS0-TP socket */
	r = conn_can_isotp_create(&conn, &isotp_opt, can_num);
	if(r < 0) {
		printf("[!] cannot create the CAN socket: err=%d\n", r);
		goto err;
	} else if(r == 0) {
		r = conn_can_isotp_bind(&conn, NULL);
		if(r < 0) {
			printf("[!] cannot bind the CAN socket: err=%d\n", r);
			goto err;
		}
		sck_ready = true;
		DEBUG("[#] The IS0-TP socket has been stared\n");
	}

err:
	return r;
}

int ihb_isotp_close(void)
{

	int r;

	if (!sck_ready)
		return -EAGAIN;

	/* Close the IS0-TP socket */
	r = conn_can_isotp_close(&conn);
	if(r < 0) {
		printf("[!] cannot close the CAN socket: err=%d\n", r);
		goto err;
	}

	DEBUG("[#] The IS0-TP socket has been closed\n");
	sck_ready = false;
	*ex_sck_ready = false;
	state_event(FAIL);
err:
	return r;
}

int ihb_isotp_send_chunks(const void *data, const size_t length)
{
	int r;

	if (!sck_ready || !data || length == 0)
		return -EAGAIN;

	DEBUG("[#] Chunk=%ubytes, data addr=%p\n", length, data);

	/*
	 * CAN_ISOTP_TX_DONT_WAIT make it not blocking.
	 */
	r = conn_can_isotp_send(&conn, data, length, 0);

	if(r < 0) {
		isotp_sender_watchdog();
		printf("[!] iso-tp send failure: err=%d\n", r);
		goto err;
	}

	if(r > 0 && (size_t)r != length) {
		printf("[!] iso-tp short send: sent %d/%ubytes\n", r, length);
		goto err;
	}

	/*
	 * Reset the watchdog if has been triggered but the isotp connection
	 * is still working.
	 */
	if(snd_chunk_fails)
		snd_chunk_fails = false;

	DEBUG("[#] The iso-tp chunk has been sent\n");

err:
	return r;
}
