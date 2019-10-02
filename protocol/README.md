# Protocol

This repository contains the source code for the Protocol, which streams in real time a desktop environment from a virtual machine (cloud computer) to a user's local client (Cube/Slate, etc.). The protocol is meant to achieve as low latency as possible to offer a fully fluid experience to users for any use (watching videos, gaming, etc.).

This repository contains both the source code for the client (running on the Cube, Slate, etc.) and for server (running on the virtual machine) to establish the connection.

In order to achieve low latency, a standard Remote Desktop Protocol (RDP) is not a valid protocol for Fractal's needs. Our protocol is based on RSTP/RTP for video transmission, utilizing the H.264 codec for video compression, and HTTPS for send user actions back to the server (virtual machine).

Here is a schematic of the protocol infrastructure, with "Game Server = Virtual Machine" and "Game Client = User Client":

![img](img/protocol_infrastructure.png)

The protocol is to be implemented in C/C++.
