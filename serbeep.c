#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

// define
#define	PORT	25252

int main(int argc, char *argv[])
{
	// UDP
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
		execl("/bin/bash", "/bin/bash", "/home/mrks/notice.sh");
	}

	close(udp);
	return 0;
}

