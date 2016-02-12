import socket
import struct
import mido

# MIDI Parser
def removeTrack(mid, tid):
	tmp = mido.MidiFile()

	for i, track in enumerate(mid.tracks):
		flg = (tid is None) or (i not in tid)

		new = mido.MidiTrack()
		for msg in track:
			if not flg and (flg or msg.type in ['note_on', 'note_off', 'polytouch']):
				continue
			new.append(msg)
		tmp.tracks.append(new)

	return tmp


def mid2pack(mid):
	top = 0
	lst = []

	for msg in mid:
		if msg.type not in ['note_on', 'note_off', 'polytouch']:
			continue

		if len(lst) == 0:
			if msg.type != 'note_on': continue
			top = msg.time
			lst.append(['on', msg.note, msg.channel, 0])
			continue

		prev = lst[-1]
		if msg.type == 'note_on':
			if prev[0] != 'off': continue
			lst.append(['on', msg.note, msg.channel, 0])
			prev[3] += msg.time
			continue

		if prev[1] != msg.note or prev[2] != msg.channel:
			continue

		if msg.type == 'note_off':
			if prev[0] != 'on': continue
			lst.append(['off', msg.note, msg.channel, 0])
			prev[3] += msg.time
			continue

		if msg.type == 'polytouch':
			if prev[0] != 'on': continue
			prev[3] += msg.time
			continue

	if len(lst) % 2 != 0:
		print "Fail"
		exit(1)

	pack = b''
	length = 0

	if top != 0:
		length += 1
		t = int(top * 1000)
		if t > 65535: t = 65535
		pack += struct.pack('>H', 0)
		pack += struct.pack('>H', 0)
		pack += struct.pack('>H', t)

	for i in range(0, len(lst), 2):
		length += 1
		if length > 65535: break

		on = lst[i]
		off = lst[i+1]

		note = on[1]
		beep_time = int(on[3] * 1000)
		sleep_time = int(off[3] * 1000)

		if beep_time > 65535: beep_time = 65535
		if sleep_time > 65535: sleep_time = 65535

		pack += struct.pack('>H', note)
		pack += struct.pack('>H', beep_time)
		pack += struct.pack('>H', sleep_time)

	return length, pack

def dumpMidi(mid, num=None):
	print mid

	for i, track in enumerate(mid.tracks):
		if (num is not None) and (i != num):
			continue

		print 'Track %d: %s' % (i, track.name)
		for message in track:
			print "\t", message


# Serbeep
MAGIC = ord("\a")

def clientHello(sock):
	data = struct.pack('>B', MAGIC) + struct.pack('>B', 0x2)
	length = sock.send(data)

	print 'Send: clientHello'
	return length == 2

def serverHello(sock):
	data = sock.recv(2)
	magic = struct.unpack('>B', data[0])[0]
	cmd = struct.unpack('>B', data[1])[0]

	print 'Recv: serverHello'
	return (magic == MAGIC) and ((cmd & 0x3) == 0x3)

def musicScore(sock, mid):
	length, pack = mid2pack(mid)

	data = struct.pack('>B', MAGIC) + struct.pack('>B', 0x4)
	data += struct.pack('>H', length)
	data += pack

	l = sock.sendall(data)
	print 'Send: musicScore'
	return (l is None)

def musicAck(sock):
	data = sock.recv(2)
	magic = struct.unpack('>B', data[0])[0]
	cmd = struct.unpack('>B', data[1])[0]

	print 'Recv: musicAck'
	return (magic == MAGIC) and ((cmd & 0x5) == 0x5)

if __name__ == '__main__':
	# Addr
	brd_addr = '10.11.39.255'
	hosts = [
		'10.11.36.225',
		'10.11.38.219',
		'10.11.36.222',
	]
	port = 25252

	# MIDI
	filename = "/home/mrks/koka.mid"
	mid = mido.MidiFile(filename)
	#dumpMidi(mid, None)
	#exit()

	# Control
	tcpsock = {}
	while True:
		cmd = raw_input('CMD> ')

		# TCP
		if cmd == 'connect':
			for host in hosts:
				tcpsock[host] = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
				tcpsock[host].connect((host, port))

		if cmd == 'close':
			for host in hosts:
				tcpsock[host].close()

		if cmd == 'hello':
			for host in hosts:
				assert clientHello(tcpsock[host]), "clientHello Error"
				assert serverHello(tcpsock[host]), "serverHello Error"

		if cmd == 'transport':
			for host in hosts:
				tmp = removeTrack(mid, None)
				assert musicScore(tcpsock[host], tmp), "musicScore Error"
				assert musicAck(tcpsock[host]), "musicAck Error"

		# UDP
		msg = None
		if cmd == 'start':
			msg = 0x2
		if cmd == 'end':
			msg = 0x4

		if msg is not None:
			sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
			sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
			sock.sendto(struct.pack('>B', MAGIC) + struct.pack('>B', msg), (brd_addr, port))
			sock.close()

