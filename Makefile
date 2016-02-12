all:
	gcc serbeep.c -o serbeep -lrt -lm -lpthread -Wall

scp:
	scp -i /home/mrks/.ssh/id_rsa serbeep 10.11.38.219:/home/mrks/serbeep
	scp -i /home/mrks/.ssh/id_rsa serbeep 10.11.36.222:/home/mrks/serbeep

