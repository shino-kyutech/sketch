#include <stdio.h>
#include <stdlib.h>

#ifndef PJT_DIM
#define PJT_DIM 128
#endif 

#if PJT_DIM < 33
#define NARROW_SKETCH
#elif PJT_DIM < 64
#define WIDE_SKETCH
#else
#define EXPANDED_SKETCH
#endif

#if defined(NARROW_SKETCH)
	#define SKETCH_SIZE 1 // スケッチの大きさ（32ビット単位）
	#define TABLE_SIZE 4  // score計算のための表関数の表の数（8ビット毎に256の大きさの表）
	typedef unsigned int sketch_type;	// スケッチの型（32ビットまで）
#elif defined(WIDE_SKETCH)
	#define SKETCH_SIZE 1 // スケッチの大きさ（64ビット単位）
	#define TABLE_SIZE 8  // score計算のための表関数の表の数（8ビット毎に256の大きさの表）
	typedef unsigned long sketch_type;	// スケッチの型（64ビットまで）
#elif defined(EXPANDED_SKETCH)
	#define SKETCH_SIZE ((PJT_DIM + 63) / 64)  // スケッチの大きさ（64ビット単位）
	#define TABLE_SIZE (SKETCH_SIZE * 8)
	typedef unsigned long sketch_type[SKETCH_SIZE];
#endif

#ifndef EXPANDED_SKETCH

static int comp_sketch(const void *a, const void *b) {

	if(*(sketch_type *)a < *(sketch_type *)b)
		return -1;
	else if(*(sketch_type *)a == *(sketch_type *)b)
		return 0;
	else
		return 1;
	
}

#else

static int comp_sketch(const void *a, const void *b) 
{
	sketch_type *x = (sketch_type *)a, *y = (sketch_type *)b;

	printf("x = %lu %lu, y = %lu %lu\n", (*x)[0], (*x)[1], (*y)[0], (*y)[1]);
	int r = 0;
	for(int j = 0; j < SKETCH_SIZE; j++) {
		if((*x)[j] < (*y)[j]) {
			r = -1;
			break;
		} else if((*x)[j] == (*y)[j]) {
			continue;
		} else {
			r = 1;
			break;
		}
	}
	printf("r = %d\n", r);
	return r;
}

#endif

int main(void)
{
	#ifndef EXPANDED_SKETCH
	sketch_type sk[5] = {2, 1, 0, 8, 1};
	#else
	sketch_type sk[5] = {
		{5, 2},
		{5, 1},
		{5, 0},
		{4, 8},
		{4, 7}
	};
	#endif

	printf("PJT_DIM = %d\n", PJT_DIM);
	printf("TABLE_SIZE = %d\n", TABLE_SIZE);
	printf("sizeof(sketch_type) = %lu\n", sizeof(sketch_type));

	for(int i = 0; i < 5; i++) {
		#ifndef EXPANDED_SKETCH
		printf("%3lu\n", (unsigned long)sk[i]);
		#else
		printf("{");
		for(int j = 0; j < SKETCH_SIZE; j++) {
			printf("%3ld", sk[i][j]);
		}
		printf("}\n");
		#endif
	}
	printf("\n");
	qsort(sk, 5, sizeof(sketch_type), comp_sketch); // skをソート
	for(int i = 0; i < 5; i++) {
		#ifndef EXPANDED_SKETCH
		printf("%3lu\n", (unsigned long)sk[i]);
		#else
		printf("{");
		for(int j = 0; j < SKETCH_SIZE; j++) {
			printf("%3lu", sk[i][j]);
		}
		printf("}\n");
		#endif
	}
	printf("\n");
	return 0;
}

