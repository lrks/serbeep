/******************************************************************************/
/*                                                                            */
/*                                -- Serbeep --                               */
/*                                                                            */
/******************************************************************************/
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
#include <pthread.h>

#define	MAGIC	'\a'
#define	PORT	25252
#define	BACKLOG	5
#define	NANONANO	1000000000
#define CLOCK_TICK_RATE 1193180
#define	DEVICE_CONSOLE	"/dev/tty0"
#define	freeNull(ptr)	free(ptr);ptr = NULL;

/*------------------------------------*/
/*               struct               */
/*------------------------------------*/
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

/*------------------------------------*/
/*           Global variable          */
/*------------------------------------*/
struct serbeep_score_header global_score_header;
struct serbeep_score_note *global_score_notes = NULL;
pthread_mutex_t global_score_mutex = PTHREAD_MUTEX_INITIALIZER;

int global_play_state = 0;
pthread_mutex_t global_play_mutex = PTHREAD_MUTEX_INITIALIZER;


/*----------------------------------------------------------------------------*/
/*                                    Main                                    */
/*----------------------------------------------------------------------------*/
void tcpListener(void *args);
void udpListener(void *args);
int main(int argc, char *argv[])
{
	pthread_t tcp_thread;
	if (pthread_create(&tcp_thread, NULL, (void *)tcpListener, (void *)NULL) != 0) {
		perror("TCP Thread");
		return EXIT_FAILURE;
	}

	pthread_t udp_thread;
	if (pthread_create(&udp_thread, NULL, (void *)udpListener, (void *)NULL) != 0) {
		perror("UDP Thread");
		return EXIT_FAILURE;
	}

	if (pthread_join(tcp_thread, NULL) != 0) {
		perror("TCP Thread join");
		return EXIT_FAILURE;
	}

	if (pthread_join(udp_thread, NULL) != 0) {
		perror("UDP Thread join");
		return EXIT_FAILURE;
	}

	return 0;
}


/*----------------------------------------------------------------------------*/
/*                               Network Thread                               */
/*----------------------------------------------------------------------------*/
int msgHandler(int sock);
void tcpListener(void *args)
{
	struct sockaddr_in addr;
	int sock0 = socket(AF_INET, SOCK_STREAM, 0);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = INADDR_ANY;

	int optval = 1;
	bind(sock0, (struct sockaddr *)&addr, sizeof(addr));
	setsockopt(sock0, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	listen(sock0, BACKLOG);	// Todo: この辺のエラー処理

	while (1) {
		struct sockaddr_in client;
		socklen_t socklen = sizeof(client);
		int sock = accept(sock0, (struct sockaddr *)&client, &socklen);	// Todo: エラー処理
		while (msgHandler(sock) == 0);
		close(sock);
	}

	close(sock0);
}

int readHeader(int sock, struct serbeep_header *header);
void playNotes(void *args);
void udpListener(void *args)
{
	struct sockaddr_in udp_addr;
	int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
	udp_addr.sin_family = AF_INET;
	udp_addr.sin_port = htons(PORT);
	udp_addr.sin_addr.s_addr = INADDR_ANY;

	int optval = 1;
	bind(udp_sock, (struct sockaddr *)&udp_addr, sizeof(udp_addr));	// Todo: この辺エラー処理
	setsockopt(udp_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

	pthread_t play_thread;
	while (1) {
		struct serbeep_header header;
		if (readHeader(udp_sock, &header) == 1) continue;

		// Start
		if ((header.cmd & 0x2) == 0x2) {
			if (pthread_mutex_lock(&global_play_mutex) != 0) continue;
			if (global_play_state == 1) {
				pthread_mutex_unlock(&global_play_mutex);
				continue;
			}
			global_play_state = 1;
			pthread_mutex_unlock(&global_play_mutex);

			struct timespec *end = (struct timespec *)malloc(sizeof(struct timespec));	// playNotes 内で free している
			if (end == NULL) continue;
			clock_gettime(CLOCK_MONOTONIC_RAW, end);

			if (pthread_create(&play_thread, NULL, (void *)playNotes, (void *)end) != 0) {
				perror("Play Thread");
				free(end);
				continue;
			}
			pthread_detach(play_thread);
			continue;
		}

		// End
		if ((header.cmd & 0x4) == 0x4) {
			if (pthread_mutex_lock(&global_play_mutex) != 0) continue;
			if (global_play_state == 0) {
				pthread_mutex_unlock(&global_play_mutex);
				continue;
			}
			global_play_state = 0;
			pthread_mutex_unlock(&global_play_mutex);

			int fd = open(DEVICE_CONSOLE, O_WRONLY);
			ioctl(fd, KIOCSOUND, 0);
			close(fd);
			continue;
		}
	}
}


/*----------------------------------------------------------------------------*/
/*                                  Protocol                                  */
/*----------------------------------------------------------------------------*/
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

int readHeader(int sock, struct serbeep_header *header)
{
	if (readStruct(sock, header, sizeof(*header)) != 1) return 1;
	if (header->magic != MAGIC) return 1;
	if ((header->cmd & 0x1) == 0x1) return 1;

	return 0;
}

int msgHandler(int sock)
{
	// Header
	struct serbeep_header header;
	if (readHeader(sock, &header) != 0) return 1;
	if (header.magic != MAGIC) {
		fprintf(stderr, "Invalid magic\n");
		return 1;
	}

	// clientHello
	if ((header.cmd & 0x2) == 0x2) {
		// serverHello
		header.cmd |= 0x1;
		if (writeStruct(sock, &header, sizeof(header)) != 1) return 1;
		return 0;
	}

	// musicScore
	if ((header.cmd & 0x4) == 0x4) {
		if (pthread_mutex_trylock(&global_score_mutex) != 0) return 1;
		if (readStruct(sock, &global_score_header, sizeof(global_score_header)) != 1) {
			pthread_mutex_unlock(&global_score_mutex);
			return 1;
		}

		global_score_header.length = ntohs(global_score_header.length);
		size_t size = sizeof(struct serbeep_score_note) * global_score_header.length;
		global_score_notes = (struct serbeep_score_note *)malloc(size);
		if (global_score_notes == NULL) {
			fprintf(stderr, "fail...malloc(%zd bytes)\n", size);
			pthread_mutex_unlock(&global_score_mutex);
			return 1;
		}

		if (readStruct(sock, global_score_notes, size) != 1) {
			freeNull(global_score_notes);
			pthread_mutex_unlock(&global_score_mutex);
			return 1;
		}

		// musicAck
		header.cmd |= 0x1;
		if (writeStruct(sock, &header, sizeof(header)) != 1) {
			freeNull(global_score_notes);
			pthread_mutex_unlock(&global_score_mutex);
			return 1;
		}

		pthread_mutex_unlock(&global_score_mutex);
		return 0;
	}

	return 0;	// Success
}


/*----------------------------------------------------------------------------*/
/*                                    Beep                                    */
/*----------------------------------------------------------------------------*/
int goodSleep(struct timespec *end, uint16_t ms)
{
	// OKIRU
	if (pthread_mutex_lock(&global_play_mutex) != 0) return 1;
	if (global_play_state != 1) return 1;
	pthread_mutex_unlock(&global_play_mutex);

	// NERU
	double time = (double)ms / (double)1000;

	time_t sec = (time_t)time;
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

	if (req.tv_sec < 0) return 0;
	nanosleep(&req, NULL);

	return 0;
}

void playNotes(void *args)
{
	struct timespec *end = (struct timespec *)args;

	if (pthread_mutex_trylock(&global_score_mutex) != 0) return;
	if (global_score_notes == NULL) {
		pthread_mutex_unlock(&global_score_mutex);
		return;
	}

	int i, fd = open(DEVICE_CONSOLE, O_WRONLY);
	int length = (int)global_score_header.length;
	struct serbeep_score_note *notes = global_score_notes;

	for (i=0; i<length; i++) {
		uint16_t number = ntohs(notes[i].number);
		uint16_t beep_time = ntohs(notes[i].length);
		uint16_t sleep_time = ntohs(notes[i].duration);

		// Beep
		if (beep_time != 0) {
			double freq = 440 * pow(2, (double)(number - 69) / (double)12);
			int val = (int)(CLOCK_TICK_RATE / freq);

			ioctl(fd, KIOCSOUND, val);
			if (goodSleep(end, beep_time) == 1) break;
			ioctl(fd, KIOCSOUND, 0);
		}

		// Sleep
		if (sleep_time != 0) {
			if (goodSleep(end, sleep_time) == 1) break;
		}
	}

	free(end);
	freeNull(global_score_notes);
	pthread_mutex_unlock(&global_score_mutex);
	close(fd);
}

