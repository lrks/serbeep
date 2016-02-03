import paramiko
import sys
import socket
import time

def ssh_mode(ssh):
	for s in ssh.values():
		print s
		s.exec_command("sudo /home/mrks/music.sh")

def udp_mode(ssh):
	for s in ssh.values():
		sftp = s.open_sftp()
		sftp.put('/home/mrks/serbeep/a.out', '/home/mrks/a.out')
		sftp.close()
		s.exec_command("chmd +x /home/mrks/a.out")
		s.exec_command("sudo /home/mrks/a.out")

	time.sleep(1)
	sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	for h in ssh.keys():
		sock.sendto("start", (h, 25252))

if __name__ == '__main__':
	host = [
		'10.11.36.225',	#2
		'10.11.37.123',	#3
		'10.11.36.222',	#4
	]

	if len(sys.argv) < 2:
		print "A"
		exit()


	# Data
	ssh = {}
	for (idx, h) in enumerate(host):
		ssh[h] = paramiko.SSHClient()
		ssh[h].set_missing_host_key_policy(paramiko.AutoAddPolicy())
		ssh[h].connect(h, username='mrks', key_filename="/home/mrks/.ssh/id_rsa")

		sftp = ssh[h].open_sftp()
		sftp.put('/home/mrks/notice.sh', '/home/mrks/music.sh')
		sftp.close()

		ssh[h].exec_command('chmod +x /home/mrks/music.sh')
		for i in range(idx * 5):
			ssh[h].exec_command('yes > /dev/null &')


	# Exec
	if sys.argv[1] == "ssh":
		print "SSH"
		ssh_mode(ssh)
	else:
		print "UDP"
		udp_mode(ssh)

	# Kill
	val = raw_input('STOP> ')
	for s in ssh.values():
		s.exec_command('sudo killall a.out')
		s.exec_command('sudo killall music.sh')
		s.exec_command('sudo kill `ps aux | grep \'music.sh\' | head -n1 | awk \'{print $2}\'`')
		s.exec_command('killall yes')

	# Close
	for h in host:
		ssh[h].close()

