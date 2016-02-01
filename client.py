import paramiko

if __name__ == '__main__':
	host = [
#		'10.11.36.215',
#		'10.11.36.225',
#		'10.11.38.163',
#		'10.11.36.222',
		'10.11.37.33',
	]

	# Auth
	ssh = {}
	for h in host:
		ssh[h] = paramiko.SSHClient()
		ssh[h].set_missing_host_key_policy(paramiko.AutoAddPolicy())
		ssh[h].connect(h, username='mrks', key_filename="/home/mrks/id_rsa")
		print "Connected", h

	# Exec
	for h in host:
		print "Exec", h
		ssh[h].exec_command("sudo bash /home/mrks/notice.sh")

	# Close
	for h in host:
		print "Close", h
		ssh[h].close()

