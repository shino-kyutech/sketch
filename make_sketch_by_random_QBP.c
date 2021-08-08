#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _OPENMP
#include <omp.h>
#endif
#include "bit_op.h"
#include "sketch.h"
#include "quick.h"
#include "pivot_selection.h"
// select pivot by random QBP: do not use collision minimization!
// Set NUM_TRIAL_QBP = 1 for random QBP
// sample is used to calculate raious of pivot
// sample is selected each time for pivot domension 
int main(int argc, char *argv[])
{
	char *sample_dataset 	= SAMPLE_DATASET;
	char *pivot_file		= PIVOT_FILE;
	fprintf(stderr, "SEED = %d\n", SEED);
	srandom(SEED);
	struct_dataset *sample_ds = read_dataset_n(1, &sample_dataset);
	int num_data = sample_ds->num_data;
	fprintf(stderr, "read sample dataset OK. the number of data = %d\n", num_data);
	pivot_type *pivot = new_pivot(QBP);
	ftr_type median = get_median(sample_ds); //中央値計算
	clock_t start,end;
	time_t s_t, e_t;
	start = clock(); s_t = time(NULL);
	fprintf(stderr, "random QBP starts: PJT_DIM = %d, SAMPLE_SIZE = %d\n", PJT_DIM, SAMPLE_SIZE);
	select_pivot_random_QBP(pivot, median, sample_ds);
	end = clock(); e_t = time(NULL);
	write_pivot(pivot_file, pivot);
	fprintf(stderr, "TIME, %.2f, %5ld, SAMPLE_SIZE, %d\n", (double)(end-start)/CLOCKS_PER_SEC, e_t - s_t, SAMPLE_SIZE);
	
	return 0;
}
