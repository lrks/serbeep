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

#define	NANONANO	1000000000
#define CLOCK_TICK_RATE 1193180

typedef struct _music {
	int val;
	time_t sec;
	long nsec;
} Music;

/*
   独自プロトコルに沿って送られてきた楽譜を構造体に変換する

            1 byte            N byte(s)       N byte(s)
   +----------------------+--------------+----------------+
   |  flag | MIDI-NoteNo  |  Length[ms]  |  Duration[ms]  |
   +----------------------+--------------+----------------+
               flag = (Length > 0xff) or (Duration > 0xff);
                                          N = flag ? 2 : 1;

   * flag は 1bit、MIDI-NoteNo は MIDI規格をそのまま利用(7bit)
   * Length == 0 が終端
     * Duration は存在しない
     * [flag | MIDI-NoteNo] や その前の Duration の値は利用していない
     * 2 ~ 3 Bytes は無駄になる
   * 65535[ms] より大きい値は 65535[ms] になる
*/
Music *recvMusic(int sock, Music *dst, int *size)
{
	uint8_t *p, byte;
	uint16_t beep_time, sleep_time;
	int width, idx = 0, length = 0;

	while (read(sock, &byte, 1) > 0) {	// これ 1byte ずつ読んだら遅いんじゃ...
		if (length >= *size) {
			*size *= 2;
			dst = (Music *)realloc(dst, sizeof(Music) * *size);
		}

		// Note
		if (idx == 0) {
			width = ((byte & 0x80) == 0x80) ? 2 : 1;
			double freq = 440 * pow(2, (double)((byte & 0x7f) - 69) / (double)12);

			idx++;
			dst[length].val = (int)(CLOCK_TICK_RATE / freq);
			continue;
		}

		// Beep time
		double sec = 0;
		int type = (idx - 1) / width;
		if (type == 0) {
			p = (uint8_t *)&beep_time;

			if (width == 1) {
				p[0] = 0x0;
				p[1] = byte;
			} else {
				p[idx - 1] = byte;
			}

			if (idx++ != (width * 1)) continue;
			if (beep_time == 0) {
				*size = length - 1;
				return dst;
			}
			sec = (double)ntohs(beep_time) / (double)1000;
		}

		// Sleep time
		if (type == 1) {
			p = (uint8_t *)&sleep_time;

			if (width == 1) {
				p[0] = 0x0;
				p[1] = byte;
			} else {
				p[idx - 3] = byte;
			}

			if (idx++ != (width * 2)) continue;

			idx = 0;
			dst[length].val = 0;
			sec = (double)ntohs(sleep_time) / (double)1000;
		}

		dst[length].sec = (time_t)sec;
		dst[length].nsec = (long)((sec - dst[length].sec) * NANONANO);
		length++;
	}

	return NULL;
}

/*
   構造体のデータを演奏する
*/
int playMusic(int fd, Music *music, int length)
{
	struct timespec end;
	clock_gettime(CLOCK_MONOTONIC_RAW, &end);

	int i;
	for (i=0; i<length; i++) {
		Music *p = &music[i];

		end.tv_sec += p->sec;
		end.tv_nsec += p->nsec;
		end.tv_sec += end.tv_nsec / NANONANO;
		end.tv_nsec = end.tv_nsec % NANONANO;

		struct timespec now, req;
		clock_gettime(CLOCK_MONOTONIC_RAW, &now);

		req.tv_sec = end.tv_sec - now.tv_sec;
		req.tv_nsec = end.tv_nsec - now.tv_nsec;

		while (req.tv_nsec < 0) {
			req.tv_sec--;
			req.tv_nsec += NANONANO;
		}

		double a = p->sec + ((double)p->nsec / (double)NANONANO);
		double b = req.tv_sec + ((double)req.tv_nsec / (double)NANONANO);

		printf("%f\t%f\t%f\n", a, b, (a - b) * 1000);

		if (req.tv_sec >= 0) {
			if (p->val != 0) {
				ioctl(fd, KIOCSOUND, p->val);
				nanosleep(&req, NULL);
				ioctl(fd, KIOCSOUND, 0);
			} else {
				nanosleep(&req, NULL);
			}
		}
	}

	return 0;
}


int main(int argc, char *argv[])
{
	/* Recv */
	int length = 256;
	Music *p = (Music *)malloc(sizeof(Music) * length);
	int sock = open("music.bin", O_RDONLY);
	p = recvMusic(sock, p, &length);
	close(sock);


	/* Pre-process */
	int fd = open("/dev/tty0", O_WRONLY);
	printf("START> ");
	getchar();


	/* Play */
	playMusic(fd, p, length);


	/* Post-process */
	close(fd);
	free(p);

	return 0;
}

