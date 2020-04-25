#!/bin/bash
#

_fail () {
	echo "[!] Err: $1"
	exit
}

_kernel_cfg () {
	sudo cat /boot/config-$(uname -r) | grep "CONFIG_CAN"
}

##
# Check if modules is loaded
_kmod_check () {
	lsmod | grep -q "can_isotp" || _fail "Kmod: can_isotp is missing"
	lsmod | grep -q "can_raw" || _fail "Kmod: can_raw is missing"
	lsmod | grep -q "vcan" || _fail "Kmod: vcan is missing"

	# Module vcan, can_raw should be always present in your kernel while
	# can_isotp can be missing. From https://github.com/hartkopp/can-isotp
	# Ensure build dependencies are installed.
	#
	# $ cd /tmp/
	# $ git clone https://github.com/hartkopp/can-isotp.git
	# $ cd can-isotp
	# $ make
	# $ sudo make modules_install
	#
	# $ insmod ./net/can/can-isotp.ko
	#
	# When the can-isotp.ko module has been installed into the Linux Kernels
	# modules directory (e.g. with 'make modules_install') the module should
	# load automatically when opening a CAN_ISOTP socket.
}

##
# Load Linux-Modules
_kmod_load() {
	sudo modprobe can_isotp || _fail "fail to load can_isotp"
	sudo modprobe can_raw || _fail "fail to load can_raw"
	sudo modprobe vcan || _fail "fail to load vcan"
}

_get_riotos () {
	cd ..
	git clone git@github.com:RIOT-OS/RIOT.git
	cd -
}

#########################################################################
