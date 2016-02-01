//#include <sys/ioctl.h>
//#include <unistd.h>
//#include <linux/kd.h>

// for Beep
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <linux/kd.h>

// for TCP
#include <sys/socket.h>
#include <netinet/in.h>

// define
#define	CLOCK_TICK_RATE	1193180	// 調べる
#define	PORT	25252

// Global



/* Utility */
void msleep(int ms)
{
	usleep(ms * 1000);
}


/* beep */
int beep(int fd, int freq, int length)
{
	int ret = ioctl(fd, KIOCSOUND, (int)(CLOCK_TICK_RATE/freq));
	if (ret < 0) return ret;

	msleep(length);
	return ioctl(fd, KIOCSOUND, 0);
}

int recv_music(int sock, char *buf, int buf_len)
{
	while (1) {
		memset(buf, '\0', buf_len);

		int len = recv(sock, buf, buf_len - 1, 0);
		if (len <= 0) return 0;
		printf("RECV: %s", buf);

		if (buf[len - 1] != '\n') continue;
	}
}

int create_udp(char *buf, int buf_len)
{
	return 0;
}


int main(int argc, char *argv[])
{
	// UDPソケット作成
	char buf[256];
	int udp = socket(AF_INET, SOCK_DGRAM, 0);

	struct sockaddr_in udp_addr;
	udp_addr.sin_family = AF_INET;
	udp_addr.sin_port = htons(PORT);
	udp_addr.sin_addr.s_addr= INADDR_ANY;

	bind(udp, (struct sockaddr *)&udp_addr, sizeof(udp_addr));

	while (1) {
		while (1) {
			memset(buf, 0, 256);
			recv(udp, buf, 256-1, 0);

			printf("Recv: %s\n", buf);
			if (strcmp("start", buf) == 0) break;
		}

		// 演奏
		int i, fd = open("/dev/console", O_WRONLY);

		/*
		int freq[] = { 523, 587, 659, 698, 784, 880, 988, 1046 };
		for (i=0; i<3; i++) {
			beep(fd, freq[i], 100);
			msleep(100);
		}
		*/
		beep(fd, 440, 100);
		close(fd);
	}

	close(udp);
	return 0;
}

