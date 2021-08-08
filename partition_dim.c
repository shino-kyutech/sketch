#include <stdio.h>

//typedef enum {MAIN_MEMORY, SECONDARY_MEMORY} ftr_on;

/*
#define FTR_DIM 4096
#ifndef PJT_DIM
#define PJT_DIM 18
#endif
*/

#define FTR_DIM 4096
#define PJT_DIM 64
//#define NUM_PART 6			// number of partitioned spaces
#define PARTITION_TYPE_PQBP
#ifndef NUM_PART
#define NUM_PART PJT_DIM
#endif

#define PART_PJT_MIN (PJT_DIM / NUM_PART)
#define PART_PJT_MAX ((PJT_DIM + NUM_PART - 1) / NUM_PART)

// PART_NUM(p) = 射影次元（p = 0, ... , PJT_DIM - 1）に対応する部分空間番号
#define PART_NUM(p) (((p) / PART_PJT_MAX) < (PJT_DIM % NUM_PART) ? ((p) / PART_PJT_MAX) : ((p) - PJT_DIM % NUM_PART) / PART_PJT_MIN)
// PART_START(p), PART_DIM(p), PART_END(p), PART_PJT_DIM(p) 射影次元（p = 0, ... , PJT_DIM - 1）に対応する部分空間の
// 開始次元番号, 次元数, 最終次元番号, 射影次元数
#define PART_START(j) ((FTR_DIM / NUM_PART) * PART_NUM(j) + (PART_NUM(j) < FTR_DIM % NUM_PART ? PART_NUM(j) : FTR_DIM % NUM_PART))
#define PART_DIM(j) ((FTR_DIM / NUM_PART) + (PART_NUM(j) < FTR_DIM % NUM_PART ? 1 : 0))
#define PART_END(j) (PART_START(j) + PART_DIM(j) - 1)
#define PART_PJT_DIM(j) ((PJT_DIM / NUM_PART) + (PART_NUM(j) < PJT_DIM % NUM_PART ? 1 : 0))

/*
p_dim   part_num        part_start      part_end        part_dim        part_pjt_dim
0 - 3   0               0               10              11              4
4 - 7   1               11              21              11              4
8 - 10  2               22              32              11              3
11 - 13 3               33              43              11              3
14 - 16 4               44              53              10              3
17 - 19 5               54              63              10              3
*/

int main(void)
{
//	printf("p_n\tp_s\tp_e\tp_d\tp_p\n");
//	for(int j = 0; j < NUM_PART; j++) {
//		printf("%d\t%d\t%d\t%d\t%d\n", j, PART_START(j), PART_END(j), PART_DIM(j), PART_PJT_DIM(j));
//	}
	printf("p_dim\tpart_num\tpart_start\tpart_end\tpart_dim\tpart_pjt_dim\n");
	for(int j = 0; j < PJT_DIM; j++) {
		printf("%d\t%d\t\t%d\t\t%d\t\t%d\t\t%d\n", j, PART_NUM(j), PART_START(j), PART_END(j), PART_DIM(j), PART_PJT_DIM(j));
	}
			
	return 0;

}