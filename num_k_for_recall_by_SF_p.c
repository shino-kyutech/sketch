#include <stdio.h>
#include <string.h>
#include "bit_op.h"
#include "config.h"
#include "ftr.h"
#include "kNN_search.h"
#include "sketch.h"
#include <unistd.h>
#include <sys/resource.h>

static double e_time(struct timespec *start, struct timespec *end)
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
static void use_system(char *rs)
{
	char command[500];

	fflush(stdout);
	sprintf(command, "grep %s /proc/%d/status", rs, getpid());
	system(command);
	fflush(stdout);
}

// バケット（bkt）を用いて，recallのために必要な候補数k'のデータ数に対する割合（k'/n）を求める
int main(int argc, char *argv[])
{
	double p_start = atof(argv[1]);
	double p_end = atof(argv[2]);
	double p_step = atof(argv[3]);
	int num_p = (p_end - p_start + 0.005) / p_step + 1;
//	printf("p_start = %lf, p_end = %lf, p_step = %lf, num_p = %d\n", p_start, p_end, p_step, num_p); exit(0);
	double pl[num_p];
	for(int i = 0; i < num_p; i++) {
		pl[i] = p_start + p_step * i;
	}
	char *pivot_file = PIVOT_FILE;
	char *bucket_filename = BUCKET_FILE;
	char *query_ftr_filename = QUERY_FILE;
	char *answer_csv_filename = ANSWER_FILE;
	int num_data, num_queries;
	
	struct timespec tp1, tp2;
	printf("PJT_DIM = %d: ", PJT_DIM);
	use_system("VmSize");
	struct_dataset *ds_query = read_dataset_n(1, &query_ftr_filename);
	num_queries = ds_query->num_data;
	printf("read query file OK. the number of queries = %d: ", num_queries);
	use_system("VmSize");
	query_type *qr = (query_type *)malloc(sizeof(query_type) * num_queries);
	for(int i = 0; i < num_queries; i++) {
		qr[i] = (query_type) { i, ds_query->ftr_id[i].ftr };
	}

	answer_type *correct_answer = read_correct_answer(answer_csv_filename, num_queries);
	printf("read correct answer OK. ");
	use_system("VmSize");

	#if defined(PARTITION_TYPE_QBP)
	pivot_type *pivot = new_pivot(QBP);
	#elif defined(PARTITION_TYPE_PQBP)
	pivot_type *pivot = new_pivot(PQBP);
	#endif
	read_pivot(pivot_file, pivot);
	printf("read pivot OK. ");
	use_system("VmSize");

//	double pl[10] = {0.6, 0.8, 1.0, 1.2, 1.4, 1.6, 1.8, 2.0, 2.2, 2.4};
//	double pl[] = {1.0, 1.2, 1.4, 1.6, 1.8, 2.0, 2.2, 2.4};
//	double pl[] = {2.0, 2.2, 2.4, 2.6, 2.8, 3.0, 3.2};
//	int num_p = sizeof(pl) / sizeof(double);
	struct_query_sketch *qs[num_p];
	answer_type *ans[num_p];
	for(int i = 0; i < num_p; i++) {
		qs[i] = (struct_query_sketch *)malloc(sizeof(struct_query_sketch) * num_queries);
		ans[i] = (answer_type *)malloc(sizeof(answer_type) * num_queries);
	}

	printf("read compact bucket ... ");
	clock_gettime(CLOCK_REALTIME, &tp1);
	struct_bucket *bucket = read_compact_bucket(bucket_filename);
	num_data = bucket->num_data;
	clock_gettime(CLOCK_REALTIME, &tp2);
	printf(" OK. num_data = %d (%.2lf sec): ", num_data, e_time(&tp1, &tp2));
	use_system("VmSize");

	#ifndef EXPANDED_SKETCH
	for(int i = 0; i < 10; i++) {
		printf("i = %d, sk, %lu\n", i, bucket->sk[i]);
	}
//	exit(0);
	#endif

	fprintf(stderr, "set query sketch ... ");
	clock_gettime(CLOCK_REALTIME, &tp1);
	#if PRIORITY == 0
	make_bitcnt_tbl(8);
	#endif
	#pragma omp parallel for
	for(int i = 0; i < num_p; i++) {
		for(int q = 0; q < num_queries; q++) {
//			printf("q = %d, p = %1.2lf\n", q, pl[i]);
			set_query_sketch_p(&qs[i][q], &qr[q], pivot, pl[i]);
//			printf("set_query_sketch OK, q = %d\n", q);
			ans[i][q].dist = priority(bucket->sk[correct_answer[q].data_num], &qs[i][q]); // 正解データの priority
		}
	}
	clock_gettime(CLOCK_REALTIME, &tp2);
	fprintf(stderr, "OK (%.2lf sec)\n", e_time(&tp1, &tp2));

	fprintf(stderr, "scanning priorities ... ");
	clock_gettime(CLOCK_REALTIME, &tp1);
	#pragma omp parallel for
	for(int q = 0; q < num_queries; q++) {
		// 質問のスケッチの順位 = 正解のpriorityより小さいpriorityを持つデータ数 + 1 を求める
		int t = omp_get_thread_num();
		if(t == 0) {
			fprintf(stderr, "t = %d, q = %d\n", t, q);
		}
		int k, e;
		dist_type p;
		for(int i = 0; i < num_p; i++) {
			for(int j = k = e = 0; j < num_data; j++) {
				if((p = priority(bucket->sk[j], &qs[i][q])) < ans[i][q].dist) k++;
				if(p == ans[i][q].dist) e++;
				if(t == 0 && j % (num_data / 100) == 0) {
					fprintf(stderr, "i = %d, processed = %d\%\r", i, j / (num_data / 100));
				}
			}
			if(t == 0) fprintf(stderr, "\n");
			ans[i][q].dist = k + e / 2;
		}
	}
	clock_gettime(CLOCK_REALTIME, &tp2);
	fprintf(stderr, "OK (%.2lf sec)\n", e_time(&tp1, &tp2));

	fprintf(stderr, "qsort answers starts ... ");
	clock_gettime(CLOCK_REALTIME, &tp1);
	for(int i = 0; i < num_p; i++) {
		qsort(ans[i], num_queries, sizeof(answer_type), comp_answer);
	}
	clock_gettime(CLOCK_REALTIME, &tp2);
	fprintf(stderr, "OK (%.2lf sec)\n", e_time(&tp1, &tp2));

	int multi_K[num_p][1000];
	for(int i = 0; i < 1000; i++) {
		for(int j = 0; j < num_p; j++) {
			multi_K[j][i] = (int)(ans[j][(int)(num_queries * i / 1000)].dist);
		}
	}

	printf("recall, %1.1lf", pl[0]);
	for(int j = 1; j < num_p; j++) {
		printf(", %1.1lf", pl[j]);
	}
	printf(", w , pivot\n");
	for(int i = 700; i <= 990; i += 10) {
		printf("%6d,", i / 10);
		for(int j = 0; j < num_p; j++) {
			int K = multi_K[j][i];
			printf("%1.8lf,", (double)K / num_data * 100);
		}
		printf("%d, %s\n", PJT_DIM, pivot_file);
	}

	return 0;
}