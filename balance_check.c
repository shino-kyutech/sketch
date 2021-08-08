#include <stdio.h>
#include <string.h>
#include "config.h"
#include "ftr.h"
#include "kNN_search.h"
#include "sketch.h"
#include "quick.h"
#include "bit_op.h"
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/resource.h>

#if defined(NARROW_SKETCH)
#define BIT_CHECK bit_check
#elif defined(WIDE_SKETCH)
#define BIT_CHECK bit_check_long
#else
#define BIT_CHECK bit_check_expanded
#endif

int main(int argc, char *argv[])
{
	char *bucket_filename = BUCKET_FILE;

	fprintf(stderr, "PJT_DIM=%d\n", PJT_DIM);
	struct_bucket *bucket_ds = read_compact_bucket(bucket_filename); // SEQUENTIAL_FILTERING or SEQUENTIAL_FILTERING_USING_BUCKET or SEQUENTIAL_FILTERING_USING_HAMMING
	fprintf(stderr, "read compact bucket OK, ");
	int num_data = bucket_ds->num_data;
	fprintf(stderr, "number of data = %d\n", num_data);
	sketch_type *sk = bucket_ds->sk;

	int nt = omp_get_max_threads();
	int on_pool[nt][PJT_DIM];
	for(int i = 0; i < nt; i++) {
		for(int j = 0; j < PJT_DIM; j++) {
			on_pool[i][j] = 0;
		}
	}
	double ratio[PJT_DIM];
	double sum = 0, sum2 = 0;
	#pragma omp parallel for
	for(int i = 0; i < num_data; i++) {
		int t = omp_get_thread_num();
		int *on = on_pool[t];
		for(int j = 0; j < PJT_DIM; j++) {
			on[j] += BIT_CHECK(sk[i], j);
		}
	}
	int on[PJT_DIM] = {0};
	for(int j = 0; j < PJT_DIM; j++) {
		for(int t = 0; t < nt; t++) {
			on[j] += on_pool[t][j];
		}
//		printf("on[%d] = %d\n", j, on[j]);
	}
	for(int j = 0; j < PJT_DIM; j++) {
		ratio[j] = (double)on[j] / num_data;
		printf("%d, %d, %lf\n", j, on[j], ratio[j]);
		sum += ratio[j];
		sum2 += ratio[j] * ratio[j];
	}

	double av = sum / PJT_DIM;
	double stdev = sqrt(sum2 / PJT_DIM - av * av);
	printf("ave = %lf, stdev = %lf\n", av, stdev);

	return 0;
}
