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

[ -x /usr/bin/isotprecv ] || \
{
		echo "[!] install isotprecv"
		exit 1;
}

[ -x /usr/bin/isotpperf ] || \
{
		echo "[!] install isotpperf"
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

sleep 2
echo "[*] Starting ISOTP CAN dump on periph $CAN"
CMD="isotprecv $CAN -s 708 -d 700 -l"
gnome-terminal -q -- $CMD

sleep 2
echo "[*] Starting ISOTP CAN perf on periph $CAN"
CMD="isotpperf $CAN -s 700 -d 708"
gnome-terminal -q -- $CMD

echo "[*] All consoles have been starded"
