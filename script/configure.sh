#!/bin/bash
#

_fail () {
	echo -e "\n[!] Err: $1"
	if [ $2 -eq 1 ]; then
		echo -e "check the err and try again\n"
		return -1;
	fi
}

_build_isotp() {
tee /tmp/conf_script/step3.sh <<EOF
#!/bin/bash
#
# Module vcan, can_raw should be always present in your kernel while can_isotp is missing.
#
# More info here:
# https://github.com/hartkopp/can-isotp
set -e

_fail () {
	echo -e "\n[!] Err: \$1"
	if [ \$2 -eq 1 ]; then
		echo -e "check the err and try again\n"
		return -1;
	fi
}

rm -rf /tmp/can-isotp/
git clone https://github.com/hartkopp/can-isotp.git /tmp/can-isotp
cd /tmp/can-isotp/
make
if [ \$? -eq 0 ]; then

	#Generate a key to sign the module
	if [ -d "/usr/src/kernels/\$(uname -r)/certs" ]; then
		cd /usr/src/kernels/\$(uname -r)/certs
	elif [ -d "/usr/src/linux-headers-\$(uname -r)/certs" ]; then
		cd /usr/src/linux-headers-\$(uname -r)/certs
	else
		_fail "Add your right path to list to continue" 1
	fi
	sudo openssl req -new -x509 -newkey rsa:2048 -keyout signing_key.pem -outform DER -out signing_key.x509 -nodes -subj "/CN=Owner/"
	cd -

	sudo make modules_install

	if [ \$? -ne 0 ]; then
		_fail "fail to install" 1
		return -1
	fi

	#sudo insmod ./net/can/can-isotp.ko
else
	_fail "fail to build isotp kmod" 1
fi
	cd -
bold=\$(tput bold)
normal=\$(tput sgr0)
echo -e "\$(tput bold)\n\nScript successfully executed. Close this shell\$(tput sgr0)"
EOF
chmod +x /tmp/conf_script/step3.sh
}

##
# Load Linux-Modules
_kmod_load() {
tee /tmp/conf_script/step4.sh <<EOF
#!/bin/bash
set -e
_fail () {
	echo -e "\n[!] Err: \$1"
	if [ \$2 -eq 1 ]; then
		echo -e "check the err and try again\n"
		return -1;
	fi
}

sudo modprobe can_isotp || _fail "fail to load can_isotp" 0
sudo modprobe can_raw || _fail "fail to load can_raw" 0
sudo modprobe vcan || _fail "fail to load vcan" 0
bold=\$(tput bold)
normal=\$(tput sgr0)
echo -e "\$(tput bold)\n\nScript successfully executed. Close this shell\$(tput sgr0)"
EOF
chmod +x /tmp/conf_script/step4.sh
}

_get_riotos() {
tee /tmp/conf_script/step1.sh <<EOF
#!/bin/bash
#
set -e

DIR="\$(pwd)/"
riot_realse="2020.07.1"
rm -rf ../../RIOT
git clone https://github.com/RIOT-OS/RIOT.git \$DIR../../RIOT
cd \$DIR../../RIOT
git checkout \$riot_realse
bold=\$(tput bold)
normal=\$(tput sgr0)
echo -e "\$(tput bold)\n\nScript successfully executed. Close this shell\$(tput sgr0)"
EOF
chmod +x /tmp/conf_script/step1.sh
}

_socket_can() {
tee /tmp/conf_script/step2.sh <<EOF
#!/bin/bash
#
set -e
_fail () {
	echo -e "\n[!] Err: \$1"
	if [ \$2 -eq 1 ]; then
		echo -e "check the err and try again\n"
		return -1;
	fi
}

PCK_VER="libsocketcan-0.0.11"
PCK_NAME="\$PCK_VER.tar.gz"
SOURCE="https://git.pengutronix.de/cgit/tools/libsocketcan/snapshot/\$PCK_NAME"

DIR="\$(pwd)/"
wget \$SOURCE -O /tmp/\$PCK_NAME
cd /tmp/
tar -xvf \$PCK_NAME
rm -rf \$PCK_NAME
cd /tmp/\$PCK_VER
./autogen.sh
./configure
./configure --build=i686-pc-linux-gnu "CFLAGS=-m32"
make
if [ \$? -eq 0 ]; then
	sudo make install
	if [ \$? -eq 0 ]; then
		sudo ldconfig /usr/local/lib
	fi
else
	_fail "fail to make" 1
fi
cd \$DIR

bold=\$(tput bold)
normal=\$(tput sgr0)
echo -e "\$(tput bold)\n\nScript successfully executed. Close this shell\$(tput sgr0)"
EOF
chmod +x /tmp/conf_script/step2.sh
}

#########################################################################

clear
echo "==============================================================================="
echo "Ensure you to have the build dependencies installed (git, cmake, make, gcc ...)"
rm -rf /tmp/conf_script/
mkdir /tmp/conf_script

if [ "$(basename $(pwd))" != "script" ]; then
	_fail "run me from script dir" 1
fi

[[ $? -eq 0 ]] && \
	{
		TITLE="[*] 1. Get RIOT-OS source"
		# Obtains the source code
		echo "$TITLE"
		_get_riotos > /dev/null
		sleep 1
	}

[[ $? -eq 0 ]] && \
	{
		TITLE="[*] 2. Install the socket CANs module"
		# It adds libsocketcan to your system:
		#  - RPM distro (fedora, centos, ...)doesn't have package yet, so you have to compile and install it.
		#  - DEB distro (ubuntu, debian, ...) have the package:
		#     sudo dpkg --add-architecture i386
		#     sudo apt-get update
		#     sudo apt-get install libsocketcan-dev:i386
		# 
		echo "$TITLE, it needs to virutualize the IHB"
		sleep 1
		_socket_can > /dev/null
	}

[[ $? -eq 0 ]] && \
	{
		TITLE="[*] 3. Build isotp kmod"
		# ISO-TP module will be added soon to kernel main line, byt now
		# it's still RFC so you need to compile and install it like module
		echo "$TITLE"
		sleep 1
		_build_isotp > /dev/null
	}

[[ $? -eq 0 ]] && \
	{
		TITLE="[*] 4. Load Kernel modules"
		echo "$TITLE"
		sleep 1
		_kmod_load > /dev/null
	}

bold=$(tput bold)
normal=$(tput sgr0)
echo -e "===============================================================================\n"
read -p "Ready to start? [Y/N]" -n 1 -r
echo ""
if [[ $REPLY =~ ^[Yy]$ ]]; then
	echo "$bold You have to continue only if each script is well processed$normal"
else
	echo "aborting..."
	exit
fi

for f in /tmp/conf_script/*
do
	#gnome-terminal -q --hide-menubar --title="PREWIEW" --command "vi $f"
	echo "Excute $bold$f$normal script in a new shell: "
	gnome-terminal -q --window --hide-menubar
	read -p "continue [Y/n]" -n 1 -r
	if [[ $REPLY =~ ^[Yy]$ ]]; then
		echo ""
	else
		echo "aborting..."
		exit
	fi
done
echo "Your system has been configured"

