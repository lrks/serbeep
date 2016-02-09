all:
	gcc serbeep.c -lrt -lm -Wall

scp:
	scp -i /home/mrks/.ssh/id_rsa a.out 10.11.38.219:/home/mrks/a.out
	scp -i /home/mrks/.ssh/id_rsa a.out 10.11.36.222:/home/mrks/a.out

