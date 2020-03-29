### gtip-thesis
Proof of concept: protocol to scan nodes and instaurate a fault-tolerant communication

#### 1 Intro
We take into account the following scenario, we have a LAN composed of many independent MCUs and each of them collects data that have to send to a master MCU node.

The master node (we call it IHB node) perform different kinds of activities, for example:
- Scanning of all Nodes that are it linked.
- Collecting the data that Nodes are sending to IHB
- Instaurate a communication via CAN interface with a HOST what receives all data that the IHB has collected by nodes that are under it

My goal is to realize a PoC of the IHB that overcomes the limits that are highlighted in the old version of this implementation, the fundamentals:
- Redesign the hardware
- Choose an environment to develop the firmware
- Introduce a fault-tolerant of the IHB node.
