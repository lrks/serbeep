import sys
import re

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


def struct(f):
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
		struct(f)

	f.close()

