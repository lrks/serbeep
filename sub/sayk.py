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


if __name__ == '__main__':
	f = open(sys.argv[1], 'r')
	if sys.argv[2] == 'launch':
		launch_time(f)
	elif sys.argv[2] == 'sleep':
		sleepsleep(f)
	f.close()

