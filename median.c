#include <stdio.h>

#include "sketch.h"
#include "quick.h"
#include <stdlib.h>
#include <limits.h>
#include <omp.h>

#include <time.h>
//#define NUM_DATA 99990000
#define NUM_DATA 100

int num_top, max_top, num_rest, min_rest;

int main(void)
{
	int num_data = NUM_DATA;
	unsigned *sc = (unsigned *)malloc(sizeof(unsigned) * num_data);
	int *idx = (int *)malloc(sizeof(int) * num_data);
	int *idx2 = (int *)malloc(sizeof(int) * num_data);
	int *idx3 = (int *)malloc(sizeof(int) * num_data);
	struct timespec tp1, tp2;
	long sec, nsec;
	#ifndef SEED
	#define SEED 1
	#endif
	srandom(SEED);
	printf("SEED = %d\n", SEED);
	for(int i = 0; i < num_data; i++) {
		sc[i] = random() % num_data;
		printf("sc[%d] = %d\n", i, sc[i]);
		idx[i] = idx2[i] = idx3[i] = i;
	}

	printf("sort starts\n");
	clock_gettime(CLOCK_REALTIME, &tp1);
	quick_sort(idx2, sc, 0, num_data - 1);
	clock_gettime(CLOCK_REALTIME, &tp2);
	sec = tp2.tv_sec - tp1.tv_sec;
	nsec = tp2.tv_nsec - tp1.tv_nsec;
	if(nsec < 0){
		sec--;
		nsec += 1000000000L;
	}
	printf("sort ends: %ld.%09ld\n", sec, nsec);
	printf("median = (sc[%d] + sc[%d]) / 2 = %lf\n", (num_data - 1) / 2, num_data / 2, (double)(sc[idx2[(num_data - 1) / 2]] + sc[idx2[num_data / 2]]) / 2);

	clock_gettime(CLOCK_REALTIME, &tp1);
	dist_type m = get_threshold_k(idx3, sc, num_data, num_data / 2, 16);
	clock_gettime(CLOCK_REALTIME, &tp2);
	sec = tp2.tv_sec - tp1.tv_sec;
	nsec = tp2.tv_nsec - tp1.tv_nsec;
	if(nsec < 0){
		sec--;
		nsec += 1000000000L;
	}
	printf("get_threshold_k ends: %ld.%09ld, median = %d\n", sec, nsec, m);
	return 0;
}

