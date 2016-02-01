import socket
import time

if __name__ == '__main__':
	host = [
		'10.11.36.215',
		'10.11.36.225',
		'10.11.38.163',
		'10.11.36.222',
		'10.11.36.219',
	]
	port = 25252

	sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM);
	#for h in host:
	#	sock.sendto("start", (h, port))

	for i in range(len(host)):
		for h in host[:(i+1)]:
			sock.sendto("start", (h, port))
		time.sleep(1)

