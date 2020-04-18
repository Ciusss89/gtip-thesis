#!/bin/bash
#

## Default value
#
IHB_ADDR_LIST="032 03b"
DEV="/dev/ttyACM*"
CAN="can0"

[ -x /usr/bin/picocom ] || \
{
		echo "[!] install picocom"
		exit 1;
}

[ -x /usr/bin/candump ] || \
{
		echo "[!] install candump"
		exit 1;
}

[ -x /usr/bin/isotpsniffer ] || \
{
		echo "[!] install isotpsniffer"
		exit 1;
}

###############################################################################
[ ! -z $1 ] && \
{
	echo "[*] Use CAN periph $1"
	CAN=$1
}

[ $# -gt 1 ] && {
	IHB_ADDR_LIST=""
	for ((i = 2; i <= $#; i++ )); do
		IHB_ADDR_LIST="$IHB_ADDR_LIST ${!i}"
	done
}

echo "[*] Starting RAW CAN dump on periph $CAN"
sleep 2
gnome-terminal -q -- candump $CAN -c -a

for mcu in $DEV
do
	CMD="picocom -b 115200 $mcu --imap lfcrlf"
	echo "[*] Starting new shell on: $mcu"
	sleep 2
	gnome-terminal -q -- $CMD
done

for id in $IHB_ADDR_LIST
do
	CMD="isotprecv $CAN -s $id -d 0 -l"
	echo "[*] Starting ISO-TP dump on source id 0x$id"
	sleep 2
	gnome-terminal -q -- $CMD
done
