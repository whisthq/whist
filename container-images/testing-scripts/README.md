# Testing-Scripts

These scripts are designed to provide utilities for testing out whether containers are running, what the latency is to the container host, etc. Useful for debugging

## connection_tester.py
Attempts to send UDP packets from client to host and records the delay, for troubleshooting whether timeouts could be an issue

Usage:
Client side: `python3 connection_tester.py client SERVERIP`
Server side: `python3 connection_tester.py server`