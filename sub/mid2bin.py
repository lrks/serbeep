import mido
import sys
import os
import struct

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


def dump(mid, num):
	print mid

	for i, track in enumerate(mid.tracks):
		if (num is not None) and (i != num):
			continue

		print 'Track %d: %s' % (i, track.name)
		for message in track:
			print "\t", message


def mid2bin(mid, dst):
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

	for i in range(0, len(lst), 2):
		on = lst[i]
		off = lst[i+1]

		note = on[1]
		beep_time = int(on[3] * 1000)
		sleep_time = int(off[3] * 1000)

		if beep_time > 65535: beep_time = 65535
		if sleep_time > 65535: sleep_time = 65535
		flg = 0x80 if beep_time > 255 or sleep_time > 255 else 0x0

		frmt = '>B' if flg == 0x0 else '>H'
		dst.write(struct.pack('>B', note | flg))
		dst.write(struct.pack(frmt, beep_time))
		dst.write(struct.pack(frmt, sleep_time))


if __name__ == '__main__':
	filename = sys.argv[1]
	command = sys.argv[2]
	track = None if len(sys.argv) < 4 else map(int, sys.argv[3].split(','))

	mid = mido.MidiFile(filename)
	if command == 'dump':
		dump(mid, track)
	elif command == 'convert':
		tmp = removeJam(mid, track)
		b = open(filename[0:filename.rindex('.mid')] + '.bin', 'wb')
		mid2bin(tmp, b)
		b.close()


