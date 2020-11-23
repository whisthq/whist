# Container Testing Scripts

These scripts are designed to provide utilities for testing out whether containers are running, what the latency is to the container host, and so on. They are useful for debugging partially-working images and networking configurations.

## connection_tester.py

Attempts to send UDP packets from client to host and records the delay, for troubleshooting whether timeouts could be an issue in connections quality.

Usage:

```
# Client side
python3 connection_tester.py client [SERVER IP]

# Server side
python3 connection_tester.py server
```
