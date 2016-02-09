import socket
import struct
import mido

MAGIC = ord("\a")

def removeJam(mid, tid):
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


def msg(flg):
	return "Success" if flg else "Failure"

def clientHello(sock):
	data = struct.pack('>B', MAGIC) + struct.pack('>B', 0x1)
	length = sock.send(data)

	flg = (length == 2)
	print 'Send: clientHello', msg(flg)
	return flg

def serverHello(sock):
	data = sock.recv(2)
	magic = struct.unpack('>B', data[0])[0]
	cmd = struct.unpack('>B', data[1])[0]

	flg = (magic == MAGIC) and ((cmd & 0x2) == 0x2)
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

	flg = (magic == MAGIC) and ((cmd & 0x8) == 0x8)
	print 'Recv: musicAck', msg(flg)
	return flg

if __name__ == '__main__':
	host = 'localhost'
	port = 25252

	sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	sock.connect((host, port))

	# Hello
	assert clientHello(sock), "clientHello Error"
	assert serverHello(sock), "serverHello Error"

	# Music
	filename = "/home/mrks/koka.mid"
	track = None

	mid = mido.MidiFile(filename)
	tmp = removeJam(mid, track)
	assert musicScore(sock, mid), "musicScore Error"
	assert musicAck(sock), "musicAck Error"


	sock.close()

