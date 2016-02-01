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
	int udp = socket(AF_INET, SOCK_DGRAM, 0);

	struct sockaddr_in udp_addr;
	udp_addr.sin_family = AF_INET;
	udp_addr.sin_port = htons(PORT);
	udp_addr.sin_addr.s_addr= INADDR_ANY;

	bind(udp, (struct sockaddr *)&udp_addr, sizeof(udp_addr));

	memset(buf, 0, buf_len);
	recv(udp, buf, buf_len-1, 0);
	printf("%s\n", buf);

	close(udp);
}


int main(int argc, char *argv[])
{
	int sock0 = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = INADDR_ANY;	// 0.0.0.0 ?

	int val = 1;
	setsockopt(sock0, SOL_SOCKET, SO_REUSEADDR, (const char *)&val, sizeof(val));
	bind(sock0, (struct sockaddr *)&addr, sizeof(addr));
	listen(sock0, 5);	// 5...?

	while (1) {
		int fd = open("/dev/console", O_WRONLY);

		struct sockaddr_in client;
		int len = sizeof(client);
		int sock = accept(sock0, (struct sockaddr *)&client, &len);

		// 曲データ受信
		char buf[256];
		recv_music(sock, buf, 256);

		// UDPソケットを作る
		create_udp(buf, 256);

		// あと適当に
		send(sock, "Hello\n", 6, 0);
		close(sock);
		close(fd);
	}

	close(sock0);
	return 0;
}

