# Fractal Protocol

This repository contains the source code for the Fractal Protocol, which streams
in real-time a desktop environment from a virtual machine (cloud computer) to a
user's local client (Desktop OSes, iOS, Android, Cube/Slate (RPi), etc.). The
protocol is designed to achieve as low latency as possible so to offer a fully
fluid experience to users for any use, even the most deamdning (watching videos,
gaming, etc.).

This repository contains both the source code for the server, which runs
exclusively on Windows 10, and for multiple clients (Windows, MacOS, Linux, iOS,
Android, Web). The following are left to implement:

- Server
- Windows Client
- MacOS Client
- Linux Client
- iOS/iPadOS Client
- Android Client
- Web Client

In order to achieve low latency, a standard Remote Desktop Protocol (RDP) is not
efficient enough for Fractal's needs. Our protocol is based on a light UDP stream
with minimal headers (think RTP) and utilises H.264/H.265 for encoding, sending
user actions back via TCP from SDL2.

Here is a schematic of the protocol infrastructure, with "Game Server = Virtual
Machine" and "Game Client = User Client":

![img](img/protocol_infrastructure.png)

The protocol is implemented in C.
