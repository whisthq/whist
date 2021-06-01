import socket
import sys
import time

current_milli_time = lambda: int(round(time.time() * 1000))

# UDP_ports = range(32263, 32273)
# TCP_ports = range(32273, 32282)
# conversation_port = 32262

TEST_PORT = 32263


def server():
    # conversation_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # conversation_socket.bind(('', conversation_port))

    test_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    test_socket.bind(("", TEST_PORT))
    start_time = time.time()

    while time.time() - start_time < 30.0:
        data, address = test_socket.recvfrom(4096)
        print("Received %d bytes from %s" % (len(data), address))
        if len(data) > 0:
            time_delay = current_milli_time() - int.from_bytes(data, byteorder="big")
            print("Time delay is %d" % time_delay)

    print("Been doing this for 30 seconds, retiring")
    test_socket.close()


def client(dest):
    test_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    start_time = time.time()
    while time.time() - start_time < 12.0:
        print("Sending data...")
        test_socket.sendto(current_milli_time().to_bytes(8, byteorder="big"), (dest, TEST_PORT))
        time.sleep(1)


def main():
    is_server = False
    destination = ""
    if sys.argv[1] in ["client", "server"]:
        if sys.argv[1] == "server":
            is_server = True
    else:
        print("Invalid first argument " + sys.argv[1])

    if not is_server:
        try:
            destination = sys.argv[2]
        except IndexError:
            print("Need to specify the IP address to connect to")

    if is_server:
        server()
    else:
        client(destination)


if __name__ == "__main__":
    main()
