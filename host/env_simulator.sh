#!/bin/bash
#

IHB_MAGIC="5F49484205F5565F"
DEVICE="vcan0"

_init_periph() {
	modprobe vcan || {
		echo "Error to load module vcan"
		exit
	}

	ip link add dev $DEVICE type vcan
}

_periph_up() {
	ip link set up $DEVICE
}

_kill() {
	kill $PID_IHB8 $PID_IHB20 $PID_IHB101 $PID_IHB121 $PID_IHB243 $PID_IHB4
}

_clean() {
	ip link delete dev vcan0

	modprobe -r vcan
}

_simulate_notify() {
	cangen $DEVICE -I 8 -g 2500 -D $IHB_MAGIC -L 8 & PID_IHB8="$!"
	cangen $DEVICE -I 20 -g 1500 -D $IHB_MAGIC -L 8 & PID_IHB20="$!"
	cangen $DEVICE -I 65 -g 4000 -D $IHB_MAGIC -L 8 & PID_IHB101="$!"
	cangen $DEVICE -I 121 -g 5700 - & PID_IHB121="$!"
	cangen $DEVICE -I 234 -g 5900 & PID_IHB243="$!"
	cangen $DEVICE -I 4 -g 2000 -L 8 & PID_IHB4="$!"
}

_dump_can() {
	gnome-terminal -q -- candump $DEVICE -c -a
}

if [ "$EUID" -ne 0 ]
  then echo "Please run as root"
  exit
fi


_init_periph
_periph_up

_simulate_notify

_dump_can
echo "enter to exit" && read;

_kill
_clean
