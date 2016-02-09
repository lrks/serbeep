#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/kd.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <math.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#define	MAGIC	'\a'
#define	PORT	25252
#define	BACKLOG	5
#define	NANONANO	1000000000
#define CLOCK_TICK_RATE 1193180

/* struct */
struct serbeep_header {
	uint8_t magic;
	uint8_t cmd;
};

struct serbeep_score_header {
	uint16_t length;
};

struct serbeep_score_note {
	uint16_t number;
	uint16_t length;
	uint16_t duration;
};

/* beep */
void goodSleep(struct timespec *end, uint16_t ms)
{
	double time = (double)ms / (double)1000;

	time_t sec = (time_t)sec;
	long nsec = (long)((time - sec) * NANONANO);

	end->tv_sec += sec;
	end->tv_nsec += nsec;
	end->tv_sec += end->tv_nsec / NANONANO;
	end->tv_nsec = end->tv_nsec % NANONANO;

	struct timespec now, req;
	clock_gettime(CLOCK_MONOTONIC_RAW, &now);

	req.tv_sec = end->tv_sec - now.tv_sec;
	req.tv_nsec = end->tv_nsec - now.tv_nsec;

	while (req.tv_nsec < 0) {
		req.tv_sec--;
		req.tv_nsec += NANONANO;
	}

	if (req.tv_sec < 0) return;

	double a = time;
	double b = req.tv_sec + ((double)req.tv_nsec / (double)NANONANO);
	printf("ERR: %f[ms]\n", (a - b) * 1000);
	nanosleep(&req, NULL);
}

void playNotes(int fd, struct serbeep_score_note *notes, int length)
{
	int i;
	struct timespec end;
	clock_gettime(CLOCK_MONOTONIC_RAW, &end);

	for (i=0; i<length; i++) {
		uint16_t number = ntohs(notes[i].number);
		uint16_t beep_time = ntohs(notes[i].length);
		uint16_t sleep_time = ntohs(notes[i].length);

		// Beep
		if (beep_time != 0) {
			double freq = 440 * pow(2, (double)(number - 69) / (double)12);
			int val = (int)(CLOCK_TICK_RATE / freq);

			ioctl(fd, KIOCSOUND, val);
			goodSleep(&end, beep_time);
			ioctl(fd, KIOCSOUND, 0);
		}

		// Sleep
		if (sleep_time != 0) {
			goodSleep(&end, sleep_time);
		}
	}
}


/* Socket */
int readStruct(int sock, void *struct_pointer, size_t size)
{
	size_t n = 0;
	uint8_t *p = (uint8_t *)struct_pointer;

	while (n < size) {
		ssize_t len = read(sock, p+n, size-n);

		switch (len) {
		case -1:
			perror("readStruct()");
			return -1;
		case 0:
			//fprintf(stderr, "EOF\n");
			return 0;
		}

		n += len;
	}

	return 1;
}

int writeStruct(int sock, void *struct_pointer, size_t size)
{
	size_t n = 0;
	uint8_t *p = (uint8_t *)struct_pointer;

	while (n < size) {
		ssize_t len = write(sock, p+n, size-n);

		switch (len) {
		case -1:
			perror("writeStruct()");
			return -1;
		case 0:
			fprintf(stderr, "something happened\n");
			return 0;
		}

		n += len;
	}

	return 1;
}

int tcpHandler(int sock, struct serbeep_score_header *score_header, struct serbeep_score_note **score_notes)
{
	int flg;

	// Header
	struct serbeep_header header;
	flg = readStruct(sock, &header, sizeof(header));
	if (flg != 1) return 1;
	if (header.magic != MAGIC) {
		fprintf(stderr, "Invalid magic\n");
		return 1;
	}

	// clientHello
	if ((header.cmd & 0x1) == 0x1) {
		// serverHello
		header.cmd = 0x2;
		flg = writeStruct(sock, &header, sizeof(header));
		if (flg != 1) return 1;
	}

	// musicScore
	if ((header.cmd & 0x4) == 0x4) {
		flg = readStruct(sock, score_header, sizeof(*score_header));
		if (flg != 1) return 1;

		score_header->length = ntohs(score_header->length);
		size_t size = sizeof(struct serbeep_score_note) * score_header->length;
		*score_notes = (struct serbeep_score_note *)malloc(size);
		if (*score_notes == NULL) {
			fprintf(stderr, "fail...malloc(%zd bytes)\n", size);
			return 1;
		}

		flg = readStruct(sock, *score_notes, size);
		if (flg != 1) return 2;

		// musicAck
		header.cmd = 0x8;
		flg = writeStruct(sock, &header, sizeof(header));
		if (flg != 1) return 2;
	}

	return 0;	// Success
}


int main(int argc, char *argv[])
{
	// Music
	struct serbeep_score_header score_header;
	struct serbeep_score_note *score_notes;

	// UDP
	struct sockaddr_in udp_addr;
	int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
	udp_addr.sin_family = AF_INET;
	udp_addr.sin_port = htons(PORT);
	udp_addr.sin_addr.s_addr = INADDR_ANY;
	bind(udp_sock, (struct sockaddr *)&udp_addr, sizeof(udp_addr));

	// TCP
	struct sockaddr_in addr;
	int sock0 = socket(AF_INET, SOCK_STREAM, 0);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = INADDR_ANY;

	int optval = 1;
	setsockopt(sock0, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	bind(sock0, (struct sockaddr *)&addr, sizeof(addr));
	listen(sock0, BACKLOG);

	while (1) {
		struct sockaddr_in client;
		socklen_t socklen = sizeof(client);
		int sock = accept(sock0, (struct sockaddr *)&client, &socklen);	// 直しまくる

		while (1) {
			int flg = tcpHandler(sock, &score_header, &score_notes);
			if (flg == 0) continue;
			if (flg == 2) free(score_notes);
			break;
		}

		close(sock);
		break;	// imadake
	}

	printf("Length = %u\n", score_header.length);

	// Beep
	int fd = open("/dev/tty0", O_WRONLY);
	//readStruct(udp_sock,
	printf("START> ");
	getchar();

	playNotes(fd, score_notes, (int)score_header.length);

	close(fd);
	free(score_notes);	// 気を付ける、二重free、NULLいれておく
	return 0;
}

