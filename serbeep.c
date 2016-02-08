#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/kd.h>

#define	NANONANO	1000000000

typedef struct _music {
	int val;
	time_t sec;
	long nsec;
	struct _music *next;
} Music;

int main(int argc, char *argv[])
{
	int length;
	Music music[512];
	#include "music.dat"

	int fd = open("/dev/tty0", O_WRONLY);

	printf("START> ");
	getchar();

	Music *p = &music[0];

	struct timespec end;
	clock_gettime(CLOCK_MONOTONIC_RAW, &end);

	int i;
	for (i=0 i<length; i++) {
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

	close(fd);
	return 0;
}

