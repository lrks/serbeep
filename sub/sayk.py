import sys
import re
import math
import struct

def getsec(string):
	s = string.split(':')
	return int(s[0]) * 3600 + int(s[1]) * 60 + float(s[2])

def launch_time(f):
	start = 0.0
	needle = 'execve'
	line = 1

	for l in f:
		if needle not in l:
			continue

		if needle == 'execve':
			if 'beep' in l:
				needle = 'ioctl(3, KIOCSOUND,'
			elif 'sleep' in l:
				needle = 'nanosleep'
			else:
				continue

			start = getsec(l.split()[1])
			continue

		needle = 'execve'
		print '%d %f' % (line, getsec(l.split()[1]) - start)
		line += 1

def sleepsleep(f):
	line = 1
	r = re.compile("nanosleep\({(.*?), (.*?)}")

	for l in f:
		if 'nanosleep' not in l:
			continue

		m = r.search(l)
		tv_sec = m.group(1)
		tv_nsec = m.group(2).zfill(9)
		print '%d %f' % (line, float(l.split('<')[1][:-2]) - float(tv_sec + '.' + tv_nsec))
		line += 1

def script(f):
	string = 'beep'
	first_beep = False

	for l in f:
		if 'beep' in l: first_beep = True
		if not first_beep: continue

		if 'beep' in l:
			string += ' ' + l[5:].strip()
		elif 'sleep' in l:
			string += ' -D %f -n' % (float(l[6:].strip()) * 1000)
		else:
			continue

	print '#!/bin/bash'
	print string

def toption(f):
	ioctl_einval_time = []
	ioctl_beepon_time = []
	ioctl_beepoff_time = []
	open_time = []
	close_time = []
	devtty = False

	r = re.compile("<([0-9.]*?)>")
	for l in f:
		m = r.search(l)
		if m is None: continue
		t = float(m.group(1))

		if 'ioctl' in l:
			if 'EINVAL' in l:
				ioctl_einval_time.append(t)
			elif 'ioctl(3, KIOCSOUND, 0)' in l:
				ioctl_beepoff_time.append(t)
			else:
				ioctl_beepon_time.append(t)
		elif 'open("/dev/tty0", O_WRONLY)' in l and '= 3' in l:
			open_time.append(t)
			devtty = True
		elif devtty and 'close(3)' in l:
			close_time.append(t)
			devtty = False

	print 'open, ioctl(EVIOCGSND(0)), ioctl(BeepOn), ioctl(BeepOff), close'
	for (a, b, c, d, e) in zip(open_time, ioctl_einval_time, ioctl_beepon_time, ioctl_beepoff_time, close_time):
		print '%f, %f, %f, %f, %f' % (a, b, c, d, e)


def cstruct(f):
	string = 'beep'
	first_beep = False

	idx = 0
	for l in f:
		if 'beep' in l: first_beep = True
		if not first_beep: continue

		val = sec = nsec = None
		if 'beep' in l:
			tmp = l[5:].split()
			time = float(tmp[3]) / 1000
			val = int(1193180 / float(tmp[1]))
		elif 'sleep' in l:
			time = float(l[6:].strip())
			val = 0
		else:
			continue

		sec = int(time)
		nsec = int((time - sec) * (10 ** 9))

		print ""
		print "music[%d].val = %d;" % (idx, val)
		print "music[%d].sec = %d;" % (idx, sec)
		print "music[%d].nsec = %d;" % (idx, nsec)

		idx += 1
	print "length = %d;" % idx


def binary(f):
	string = 'beep'
	first_beep = False

	idx = 0
	lst = []
	before = 'sleep'
	for l in f:
		if 'beep' in l: first_beep = True
		if not first_beep: continue

		time = note = None
		if 'beep' in l:
			tmp = l[5:].split()
			time = float(tmp[3])
			freq = int(tmp[1])
			note = int(12 * math.log(float(freq) / 440.0, 2) + 69)
			if note < 0:
				note = 0
			if note > 127:
				note = 127
		elif 'sleep' in l:
			time = float(l[6:].strip()) * 1000
		else:
			continue

		if (note is None) == (before == 'sleep'):
			print "ERROR"
			exit()

		if time < 0:
			time = 0
		elif time > 65535:
			time = 65535

		if note is not None:
			before = 'beep'
			lst.append([note, time])
		else:
			before = 'sleep'
			lst[idx].append(time)
			idx += 1

	b = open('music.bin', 'wb')
	for l in lst:
		beep = int(l[1])
		sleep = 0 if len(l) == 2 else int(l[2])
		note = int(l[0])
		flag = 0x80 if beep > 255 or sleep > 255 else 0x0
		frmt = '>B' if flag == 0x0 else '>H'

		b.write(struct.pack('>B', note | flag))
		b.write(struct.pack(frmt, beep))
		b.write(struct.pack(frmt, sleep))
		b.flush()


	b.write(struct.pack('>B', 0))
	b.write(struct.pack('>B', 0))

	b.close()


if __name__ == '__main__':
	f = open(sys.argv[1], 'r')

	if sys.argv[2] == 'launch':
		launch_time(f)
	elif sys.argv[2] == 'sleep':
		sleepsleep(f)
	elif sys.argv[2] == 'script':
		script(f)
	elif sys.argv[2] == 'toption':
		toption(f)
	elif sys.argv[2] == 'struct':
		cstruct(f)
	elif sys.argv[2] == 'binary':
		binary(f)

	f.close()

