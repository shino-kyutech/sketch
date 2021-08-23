#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _OPENMP
#include <omp.h>
#endif
#include "parm.h"
#include "bit_op.h"
#include "sketch.h"
#include "quick.h"
#include "pivot_selection.h"

int main(int argc, char *argv[])
{
	#if defined(NUM_THREADS) && NUM_THREADS > 1
	omp_set_num_threads(NUM_THREADS);
	#endif
	int nt = omp_get_max_threads();
	fprintf(stderr, "number of threads = %d\n", nt);
	char *sample_dataset 	= SAMPLE_DATASET;
	char *pivot_file		= PIVOT_FILE;
	fprintf(stderr, "SEED = %d\n", SEED);
	srandom(SEED);
	struct_dataset *sample_ds = read_dataset_n(1, &sample_dataset);
	int num_data = sample_ds->num_data;
	fprintf(stderr, "read sample dataset OK. the number of data = %d\n", num_data);
	#if defined(PARTITION_TYPE_QBP)
	pivot_type *pivot = new_pivot(QBP);
	#elif defined(PARTITION_TYPE_PQBP)
	pivot_type *pivot = new_pivot(PQBP);
	#else
	#error "PARTITION_TYPE_QBP or PARTITION_TYPE_PQBP should be defined."
	#endif
	ftr_type *sample = get_sample(sample_ds); // ピボット評価用のサンプル
	ftr_type median = get_median(sample_ds); //中央値計算

	for(int i = 0; i < FTR_DIM; i++) {
		printf("med[%d] = %d\n", i, median[i]);
	}
	
	clock_t start,end;
	time_t s_t, e_t;
	start = clock(); s_t = time(NULL);
	fprintf(stderr, "QBP starts: NUM_TRIAL_QBP = %d\n", NUM_TRIAL_QBP);
	select_pivot_QBP(pivot, median, sample, sample_ds, nt);
	end = clock(); e_t = time(NULL);
	write_pivot(pivot_file, pivot);
	fprintf(stderr, "TIME, %.2f, %5ld, NUM_TRIAL_QBP, %d\n", (double)(end-start)/CLOCKS_PER_SEC, e_t - s_t, NUM_TRIAL_QBP);
	
	return 0;
}
