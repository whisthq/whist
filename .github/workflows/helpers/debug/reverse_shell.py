import sys, platform
import socket, select, time
import os, threading
import subprocess as sp
import signal

# See https://github.com/whisthq/whist/pull/7474 for usage

# See https://github.com/whisthq/whist/wiki/upgrade_shell for upgrade the shell
# e.g. make Ctrl-C, TAB completion, ls color print, ... work

# WARNING: this rever shell doesn't have encrpytion.
# Don't enable it on CI by default,
# Don't try to use it as an alternative of SSH for daily use,
# Only enable when needed, e.g. debug tough CI/workflow problems.


# the seconds for an idle quit
IDLE_DISCONNECT_TIME_SEC = 600

if sys.version_info.major < 3:
    print("need python3 to run")
    exit(0)

if len(sys.argv) != 3:
    print("invalid argument", sys.argv)
    exit(0)

# allow quit this program by signal
signal.signal(signal.SIGINT, signal.SIG_DFL)

print("OS is", platform.system())
ip = sys.argv[1]
port = int(sys.argv[2])
print("IP=", ip, ", ", "port=", port)

# decide the shell command based on OS
cmd_arr = ["bash", "-i"]
if platform.system() == "Windows":
    cmd_arr = ["cmd.exe"]

# open a subprocess of shell command, capture the stdin and stdout
# and with stderr redirected to stdout
p = sp.Popen(cmd_arr, stdin=sp.PIPE, stdout=sp.PIPE, stderr=sp.STDOUT)

# connect to the "server"
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
# make connect() timeout in 5 seconds
s.settimeout(5)
try:
    s.connect((ip, port))
except:
    print("failed to connect")
    exit(0)

print("connected")

# after connected, change timeout back, for better compatiblity
# note: for python3 < 3.7, a positive timeout will set the underlying socket to non-blocking,
# which can break the behavior of something like p.stdout.fileno()
s.settimeout(None)
s.send("hello message from client!!!!\n".encode())


def stdout_thread():
    while True:
        o = os.read(p.stdout.fileno(), 1024)
        s.send(o)


# use a thread to copy stdout to socket
threading.Thread(target=stdout_thread, daemon=True).start()

last_active_time = time.time()


def stdin_thread():
    global last_active_time
    while True:
        i = s.recv(1024)
        # a zero recv of TCP indicates EOF
        if len(i) == 0:
            print("quit by EOF")
            exit(0)
        last_active_time = time.time()
        os.write(p.stdin.fileno(), i)


# use another thread to copy from socket to stdin
threading.Thread(target=stdin_thread, daemon=True).start()

# hanlde timeout by last active time
while True:
    print("current_time:", time.time(), ",  last active time:", last_active_time)
    sys.stdout.flush()
    if time.time() - last_active_time > IDLE_DISCONNECT_TIME_SEC:
        print("quit by idle")
        exit(0)
    time.sleep(5)
