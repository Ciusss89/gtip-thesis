#!/bin/bash
#

# The ST Nucleo' board includes an on-board the ST-LINK V2 programmer that
# provides also the DEBUG interface by the MCU aurt.

DEV="/dev/ttyACM*"

[ -x /usr/bin/picocom ] || \
	{
		echo "[!] install picocom"
		exit 1;
	}

for mcu in $DEV
do
	CMD="picocom -b 115200 $mcu --imap lfcrlf"
	echo "[*] Starting new shell on: $mcu"
	sleep 2
	gnome-terminal -q -- $CMD
done
