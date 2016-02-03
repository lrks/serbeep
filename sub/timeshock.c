#include <stdlib.h>
#include <stdio.h>
#include <time.h>

void printime(int num)
{
	struct timespec tp;
	clock_gettime(CLOCK_REALTIME, &tp);
	printf("%d %ld.%09ld\n", num, tp.tv_sec, tp.tv_nsec);
	fflush(stdout);
}

int main(int argc, char *argv[])
{
	if (argc != 3) return 1;

	int step = atoi(argv[2]);
	if (step <= 0) return 1;

	int num = 0;
	char buf[256];
	FILE *fp = fopen(argv[1], "r");
	while (fgets(buf, 256, fp) != NULL) {
		if (!(buf[0] == 'b' || buf[0] == 's')) continue;

		if ((num % step) == 0) printime(num);
		system(buf);
		num++;
	}
	printime(num);

	return 0;
}

