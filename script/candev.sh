#!/bin/bash
# Author Giuseppe Tipaldi 2020

#
# https://canable.io/getting-started.html

udev_usbtty_scan()
{
	shopt -s nullglob
	for i in /dev/ttyUSB*; do
		udevadm info -r -q all "$i" | awk -F= '
		/DEVNAME/{d=$2}
		/ID_VENDOR_ID/{v=$2}
		/ID_MODEL_ID/{m=$2}
		d&&v&&m{print d,v":"m;d=v=m=""}
		'
	done
}

get_tty()
{
	local device=$1
	local path_device=$(udev_usbtty_scan | \
			    grep "$device"  | \
			    awk 'BEGIN{FS=" "} {print $1}')

	echo "$path_device"
}


validate_setup()
{

	case "$1" in
		1a86:7523)
			local DEVICE="QinHeng Electronics HL-340 USB-Serial adapter"
			;;
		*)
			echo "unkwon device"
			exit
			;;
	esac

	case "$3" in
		0)
			local sp="10k"
			;;
		1)
			local sp="20k"
			;;
		2)
			local sp="50k"
			;;
		3)
			local sp="100k"
			;;
		4)
			local sp="125k"
			;;
		5)
			local sp="250k"
			;;
		6)
			local sp="500k"
			;;
		7)
			local sp="750k"
			;;
		8)
			local sp="750k"
			;;
		*)
			echo "bad speed"
			exit;
			;;
	esac

	case "$4" in
		9600)
			local b="9600 baud"
			;;
		19200)
			local b="19200 baud"
			;;
		38400)
			local b="38400 baud"
			;;
		57600)
			local b="57600 baud"
			;;
		115200)
			local b="115200 baud"
			;;
		230400)
			local b="230400 baud"
			;;
		460800)
			local b="460800 baud"
			;;
		500000)
			local b="500000 baud"
			;;
		576000)
			local b="576000 baud"
			;;
		1000000)
			local b="1000000 baud"
			;;
		1152000)
			local b="1152000 baud"
			;;
		1500000)
			local b="1500000 baud"
			;;
		2000000)
			local b="2000000 baud"
			;;
		3000000)
			local b="3000000 baud"
			;;
		*)

			echo "bad UART speed in baud"
			exit;
			;;
	esac


	case "$5" in
		none)
			local flow_c="none"
			CAN_UART_FLOW_CONTROL=""
			;;
		hw)
			local flow_c="hardware"
			;;
		sw)
			local flow_c="software"
			;;
		*)
			local flow_c="none"
			;;
	esac

	echo "CAN adapter       : $DEVICE"
	echo "CAN Identifier    : $1"
	echo "CAN UART baoud    : $b"
	echo "CAN device        : $2"
	echo "CAN speed         : $sp"
	echo "CAN flow control  : $flow_c"
}

start_slcand()
{
	[ ! -z $CAN_UART_FLOW_CONTROL ] && CAN_UART_FLOW_CONTROL="-t $CAN_UART_FLOW_CONTROL"

	echo ">>> slcand -o -s$CAN_SPEED $CAN_UART_FLOW_CONTROL -S $CAN_UART_BAUD $CAN_DEV $CAN_ID"
	slcand -o -s$CAN_SPEED $CAN_UART_FLOW_CONTROL -S $CAN_UART_BAUD $CAN_DEV $CAN_ID
}

################################################################################

USB_DEVICE="1a86:7523"

PID_SLCAND=$(pgrep -u root slcand)
CAN_ID="can0"
CAN_DEV=$(get_tty $USB_DEVICE | tr -d '\n')
CAN_SPEED="6"
CAN_UART_BAUD="3000000"
CAN_UART_FLOW_CONTROL="hw"

if [ "$EUID" -ne 0 ]
  then echo "Please run as root"
  exit
fi

if [[ "$1" == "info" ]]; then
validate_setup $USB_DEVICE \
	       $CAN_DEV $CAN_SPEED \
	       $CAN_UART_BAUD \
	       $CAN_UART_FLOW_CONTROL
exit
fi

if [[ "$1" == "k" ]]; then
	if [ ! -z "$PID_SLCAND" ]; then
		echo "slcand PID=$PID_SLCAND has been killed"
		kill $PID_SLCAND
		exit
	else
		echo "slcand isn't running"
		exit
	fi
fi

if [ -z "$CAN_DEV" ]; then
	echo "device not found"
	exit
fi

validate_setup $USB_DEVICE \
	       $CAN_DEV $CAN_SPEED \
	       $CAN_UART_BAUD \
	       $CAN_UART_FLOW_CONTROL

if [ -z "$PID_SLCAND" ]; then
	sudo modprobe can
	sudo modprobe can-raw
	sudo modprobe slcan 
	start_slcand
	#sudo slcand -s5 -S500000 $CAN_DEV can0
	ip link set up $CAN_ID
	sleep 2
	#ip link set $CAN_ID up type can bitrate 500000
	#ip link set $CAN_ID type can bitrate 500000
	ifconfig $CAN_ID
	#ifconfig $CAN_ID txqueuelen 1000
else
	echo "slcand is already running: $PID_SLCAND"
fi

#sudo grep "CONFIG_CAN_*" /boot/config-$(uname -r)
