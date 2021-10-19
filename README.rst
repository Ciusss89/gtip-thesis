===========
gtip-thesis
===========

Thesis scope:
-------------

Implement a proof of concept of fault-tolerant communication over CAN bus where
there is a single master node and multiple slave nodes.

Consider the following scenario, We have a network composed of many independent
MCUs (aka Skin nodes) and each of them continually collects data and all
traffic is received from the one master MCU node (aka IHB node)

The IHB node can be assumed as a gate that receives the data incoming by the
skin nodes network and forwards that toward a HOST which takes care of
post-processing. As a consequence, the IHB has a key role:

1. Scan the network to discover and account for all skin nodes
2. Handle these nodes and download their data.
3. Instaurate a communication over the CAN interface with the HOST that
   receives data that the IHB has collected from the skin network

The thesis goal is to realize a PoC of the IHB that overcomes the limits that
are highlighted in the old version of this implementation, the key point:

1. Redesign the hardware
2. Choose an environment to develop the firmware
3. Introduce a fault-tolerant protocol between the IHB node and the Host

Hardware:
---------

This project runs on a Nucleo-64 board based on an STM32 microcontroller (MCU).
It's mandatory to choose hardware that provides CAN. The CAN subsystem provides
the communication between IHB and HOST, this means that the HOST interfaces
itself towards the IHB using a USB-TO-CAN adapter.

The communication between IHB and its skin network has been implemented on
the SPI interface using a proprietary protocol to discover and transfer data.
Because this topic is little a bit complex from the hardware side (custom
bus design to minimize the noise of the skin network) is not easy
to develop/debug it using a standard demo board but needs a specific hardware
design.

Note:
1. CAN subsystem is composed of two parts, the CAN controller (nowadays many
   MCUs have embedded one or more CAN controllers) and the CAN transceiver.
   If the MCU doesn't provide for the CAN controller, it needs an external
   chip that provides for the CAN controller and transceiver.
2. To interface with the system, we need the CAN-USB converter. I suggest
   using any controller supported by CAN-UTILS to skip all nasty problems
   due to drivers and other time-consuming stuff

CAN interface:
~~~~~~~~~~~~~~

This project is using the CAN controller embedded into stm32l476rg MCU which
runs on *Nucleo-64 L476* board. It needs a transceiver to communicate over the
BUS, for this scope, there is the IC MCP2551. It's a perfect match because
these transceivers are mounted on a breakout and this simplifies the
prototyping test.

  .. image:: ./media/can_network.png

The transceivers need at least 4 cables, first two to supply it, other to link
the CAN TX/RX signal. In relation to the complexity of the transceivers, other
links can be required to handle POWER ON/OFF and or the SLOPE CONTROL, anyway
this depends from specs of your transceiver.
The breakout board used by MCP2551 has a 4.7KΩ resistor between its RS pin and
ground. This force the IC to operate in SLOPE-CONTROL mode, where the slew rate
(SR) is proportional to the current ouput at the the RS pin.

Protocol:
~~~~~~~~~

The protocol which has been implemented by this PoC uses a standard CAN 2A
frames for the discovery of IHBs, while a CAN ISO-TP is used to transfer DATAs
incoming from MCU to HOST (the computer which is running the *ihbtool*
application

Firmware MCU:
~~~~~~~~~~~~~

The directory `fw` contains the source code of firmware that runs on the IHB.
It's based on RIOT-OS and it's composed of 2 main module:

1. The *IHB-CAN* is the main module, it configures the CAN subsystem, handles
   the finite state machine (FSM) and uses the standard ISO-TP to send data
   towards the HOST. There are six state (IDLE, NOTIFY, BACKUP, ACTIVE, TOFIX,
   ERR). This module is composed by two drivers, first cotrols the finite
   state machine while the second handles the iso-tp transmission.
2. The *IHB-NETSIM* module simulate a SKINs nodes network where each node
   collects data incoming from N tactile sensor.


  .. image:: ./media/fsm.png

Host Application:
~~~~~~~~~~~~~~~~~

The directory *host/fronted* contains the source code of the *ihbtool* app. It
uses the Linux socket and it's base on the CAN-utils code.

Riot-OS
~~~~~~~

The firmware is based on `Riot-OS <https://doc.riot-os.org/>`__. It has a small
footprint high hardware abstraction and is connectivity oriented.
In these last 3 months I gave a small contribution to the community:

1. `Add SPI transciver to lwip package <https://github.com/RIOT-OS/RIOT/pull/13092>`__
2. `Add CAN support for nucleo-l476rg <https://github.com/RIOT-OS/RIOT/pull/13534>`__
3. `Add dd remote request frame test <https://github.com/RIOT-OS/RIOT/pull/1373>`__
