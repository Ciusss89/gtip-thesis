#!/bin/bash
#

_fail () {
	echo -e "\n[!] Err: $1"
	if [ $2 -eq 1 ]; then
		echo -e "check the err and try again\n"
		return -1;
	fi
}

_kernel_cfg () {
	(sudo cat /boot/config-$(uname -r) | grep "CONFIG_CAN" ) > CONFIG_CAN.modules
	echo "List saved on file: CONFIG_CAN.module"
}

##
# Check if modules is loaded
_kmod_check () {
	lsmod | grep "can_isotp"
	lsmod | grep "can_raw"
	lsmod | grep "vcan"
}

_build_isotp() {
	# Module vcan, can_raw should be always present in your kernel while can_isotp is missing.
	#
	# More info here:
	# https://github.com/hartkopp/can-isotp

	rm -rf /tmp/can-isotp/
	git clone https://github.com/hartkopp/can-isotp.git /tmp/can-isotp
	cd /tmp/can-isotp/
	make
	if [ $? -eq 0 ]; then

		#Generate a key to sign the module
		if [ -d "/usr/src/kernels/$(uname -r)/certs" ]; then
			cd /usr/src/kernels/$(uname -r)/certs
		elif [ -d "/usr/src/linux-headers-$(uname -r)/certs" ]; then
			cd /usr/src/linux-headers-$(uname -r)/certs
		else
			_fail "Add your right path to list to continue" 1
		fi
		sudo openssl req -new -x509 -newkey rsa:2048 -keyout signing_key.pem -outform DER -out signing_key.x509 -nodes -subj "/CN=Owner/"
		cd -

		sudo make modules_install

		if [ $? -ne 0 ]; then
			_fail "fail to install" 1
			return -1
		fi

		#sudo insmod ./net/can/can-isotp.ko
	else
		_fail "fail to build isotp kmod" 1
	fi
	cd -
}

##
# Load Linux-Modules
_kmod_load() {
	sudo modprobe can_isotp || _fail "fail to load can_isotp" 0
	sudo modprobe can_raw || _fail "fail to load can_raw" 0
	sudo modprobe vcan || _fail "fail to load vcan" 0
}

_get_riotos () {
	rm -rf ../../RIOT
	git clone git@github.com:RIOT-OS/RIOT.git ../../RIOT
	if [ $? -ne 0 ]; then
		_fail "Get Riot-OS source fails" 1
	fi
}

_socket_can() {
	PCK_VER="libsocketcan-0.0.11"
	PCK_NAME="$PCK_VER.tar.gz"
	SOURCE="https://git.pengutronix.de/cgit/tools/libsocketcan/snapshot/$PCK_NAME"

	local DIR=$(pwd)
	wget $SOURCE -O /tmp/$PCK_NAME
	cd /tmp/
	tar -xvf $PCK_NAME
	rm -rf $PCK_NAME
	cd /tmp/$PCK_VER
	./autogen.sh
	./configure
	./configure --build=i686-pc-linux-gnu "CFLAGS=-m32"
	make
	if [ $? -eq 0 ]; then
		sudo make install
		if [ $? -eq 0 ]; then
			sudo ldconfig /usr/local/lib
		fi
	else
		_fail "fail to make" 1
	fi
	cd $DIR
}

#########################################################################

clear

echo "Ensure build dependencies are installed (git, cmake, make, gcc ...)"

if [ "$(basename $(pwd))" != "script" ]; then
	_fail "run me from script dir" 1
fi

[[ $? -eq 0 ]] && \
	{
		# Obtains the source code
		echo -e "\n[*] 1. Get Riot os source\n"
		sleep 2
		_get_riotos
	}

[[ $? -eq 0 ]] && \
	{
		# It save into CONFIG_CAN.modules file the current CANs modules.
		echo -e "\n[*] 2. show the kernel CANs module\n"
		sleep 2
		_kernel_cfg
	}

[[ $? -eq 0 ]] && \
	{
		# It adds libsocketcan to your system: 
		#  - RPM distro (fedora, centos, ...)doesn't have package yet, so you have to compile and install it.
		#  - DEB distro (ubuntu, debian, ...) have the package:
		#     sudo dpkg --add-architecture i386
		#     sudo apt-get update
		#     sudo apt-get install libsocketcan-dev:i386
		# 
		echo -e "\n[*] 3. Install socket CANs module (needs when you're running Riot-os with native board)\n"
		sleep 2
		_socket_can
	}

[[ $? -eq 0 ]] && \
	{
		# ISO-TP module will be added soon to kernel main line, byt now
		# it's still RFC so you need to compile and install it like module
		echo -e "\n[*] 4. build isotp kmod\n"
		sleep 2
		_build_isotp
	}

[[ $? -eq 0 ]] && \
	{
		echo -e "\n[*] 5. load modules\n"
		sleep 2
		_kmod_load
	}

[[ $? -eq 0 ]] && \
	{
		echo -e "\n[*] 6. module check\n"
		sleep 2
		_kmod_check
	}
cd ..
