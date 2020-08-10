import socket
import sys
import time

current_milli_time = lambda: int(round(time.time() * 1000))

# UDP_ports = range(32263, 32273)
# TCP_ports = range(32273, 32282)
# conversation_port = 32262

test_port = 32263

def server():
	# conversation_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	# conversation_socket.bind(('', conversation_port))

	test_socket = socket.socket(socker.AF_INET, socket.SOCK_DGRAM)
	test_socket.bind(('', test_port))
	startTime = time.time()

	while time.time() - startTime > 12.0:
		data, address = test_socket.recvfrom(4096)
		print("Recieved %d bytes from %s" % (len(data), address))
		if len(data) > 0
			timeDelay = current_milli_time() - data
			print("Time delay is %d" % timeDelay)

	print("Been doing this for 12 seconds, retiring")
	test_socket.close()
	return



def client(dest):
	test_socket = socket.socket(socker.AF_INET, socket.SOCK_DGRAM)
	startTime = time.time()
	while time.time() - startTime > 12.0:
		test_socket.sendto(current_milli_time(), (dest, test_port))
		time.sleep(1)
		




def main():
	is_server = False
	destination = ""
	if (sys.argv[1] in ["client", "server"]) || sys.argv[1] == "server":
		is_server = True
	else:
		print("Invalid first argument " + sys.argv[1])

	if not is_server:
		try: 
			destination = sys.argv[2]
		except IndexError,e:
			print("Need to specify the IP address to conect to")


	if is_server:
		server()
	else:
		client(destination)

if __name__ == "__main__":
	main()