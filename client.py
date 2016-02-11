import socket
import struct
import mido

# MIDI Parser
def removeTrack(mid, tid):
	tmp = mido.MidiFile()

	for i, track in enumerate(mid.tracks):
		flg = (tid is None) or (i in tid)

		new = mido.MidiTrack()
		for msg in track:
			if not flg and (flg or msg.type in ['note_on', 'note_off', 'polytouch']):
				continue
			new.append(msg)
		tmp.tracks.append(new)

	return tmp

def mid2pack(mid):
	lst = []

	for msg in mid:
		if msg.type not in ['note_on', 'note_off', 'polytouch']:
			continue

		if len(lst) == 0:
			if msg.type != 'note_on': continue
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

def dumpMidi(mid, num):
	print mid

	for i, track in enumerate(mid.tracks):
		if (num is not None) and (i != num):
			continue

		print 'Track %d: %s' % (i, track.name)
		for message in track:
			print "\t", message


# Serbeep
MAGIC = ord("\a")

def msg(flg):
	return "Success" if flg else "Failure"

def clientHello(sock):
	data = struct.pack('>B', MAGIC) + struct.pack('>B', 0x2)
	length = sock.send(data)

	flg = (length == 2)
	print 'Send: clientHello', msg(flg)
	return flg

def serverHello(sock):
	data = sock.recv(2)
	magic = struct.unpack('>B', data[0])[0]
	cmd = struct.unpack('>B', data[1])[0]

	flg = (magic == MAGIC) and ((cmd & 0x3) == 0x3)
	print 'Recv: serverHello', msg(flg)
	return flg

def musicScore(sock, mid):
	length, pack = mid2pack(mid)

	data = struct.pack('>B', MAGIC) + struct.pack('>B', 0x4)
	data += struct.pack('>H', length)
	data += pack

	l = sock.sendall(data)
	flg = (l is None)
	print 'Send: musicScore', msg(flg)
	return flg

def musicAck(sock):
	data = sock.recv(2)
	magic = struct.unpack('>B', data[0])[0]
	cmd = struct.unpack('>B', data[1])[0]

	flg = (magic == MAGIC) and ((cmd & 0x5) == 0x5)
	print 'Recv: musicAck', msg(flg)
	return flg

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
	tracks = [
		range(len(mid.tracks)),
		range(len(mid.tracks)),
		range(len(mid.tracks)),
	]

	# Transport music
	for (host, track) in zip(hosts, tracks):
		print "Host", host
		sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		sock.connect((host, port))

		# Hello
		assert clientHello(sock), "clientHello Error"
		assert serverHello(sock), "serverHello Error"

		# Music
		rm_track = list(set(range(len(mid.tracks))).difference(set(track)))
		tmp = removeTrack(mid, rm_track)
		assert musicScore(sock, tmp), "musicScore Error"
		assert musicAck(sock), "musicAck Error"

		sock.close()

	# Control
	print "Control"
	while True:
		cmd = raw_input('CMD> ')
		msg = None
		if cmd == 'start':
			msg = 0x2
		elif cmd == 'end':
			msg = 0x4
		else:
			continue

		sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
		sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
		sock.sendto(struct.pack('>B', MAGIC) + struct.pack('>B', msg), (brd_addr, port))
		sock.close()

