#!/bin/bash
#

###############################################################################
PCK_VER="libsocketcan-0.0.11"
PCK_NAME="$PCK_VER.tar.gz"
SOURCE="https://git.pengutronix.de/cgit/tools/libsocketcan/snapshot/$PCK_NAME"

echo "[*] libsocketcan"
wget $SOURCE
tar -xvf $PCK_NAME
rm -rf $PCK_NAME
cd $(pwd)/$PCK_VER
./autogen.sh
./configure
./configure --build=i686-pc-linux-gnu "CFLAGS=-m32"
make
sudo make install
sudo ldconfig /usr/local/lib
###############################################################################

sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link add dev vcan1 type vcan
sudo ip link set vcan0 up
sudo ip link set vcan1 up
