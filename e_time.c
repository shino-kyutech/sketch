// 経過時間計測および使用メモリ表示
#include <stdio.h>
#include "e_time.h"

double e_time(struct timespec *start, struct timespec *end)
{
	long sec = end->tv_sec - start->tv_sec;
	long nsec = end->tv_nsec - start->tv_nsec;
	if(nsec < 0){
		sec--;
		nsec += 1000000000L;
	}
	return (double)sec + (double)nsec/1000000000;
}

// VmSize or VmRSS
void use_system(char *rs)
{
	char command[500];

	fflush(stdout);
	sprintf(command, "grep %s /proc/%d/status", rs, getpid());
	if(system(command) == -1) {
		fprintf(stderr, "system() error.\n");
		exit(1);
	}
	fflush(stdout);
}
