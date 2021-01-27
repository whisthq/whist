# Fractal Container Testing Scripts

This subfolder contains various scripts for manually testing different components of the Fractal Docker containers (i.e. latency, connectivity, etc.). If you find yourself creating a testing script while implementing or debugging a feature of the Fractal containers, add it here so that it can help others eventually.

## Existing Scripts

### connection_tester.py

Attempts to send UDP packets from client to host and records the delay, for troubleshooting whether timeouts could be an issue in connection quality.

Usage:

```
# Client side
python3 connection_tester.py client [SERVER IP]

# Server side
python3 connection_tester.py server
```
