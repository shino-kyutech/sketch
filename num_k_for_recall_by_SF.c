#include <stdio.h>
#include <string.h>
#include "bit_op.h"
#include "config.h"
#include "ftr.h"
#include "kNN_search.h"
#include "sketch.h"

// バケット（bkt）を用いて，recallのために必要な候補数k'のデータ数に対する割合（k'/n）を求める
int main(int argc, char *argv[])
{
	char *pivot_file = PIVOT_FILE;
	char *bucket_filename = BUCKET_FILE;
	char *query_ftr_filename = QUERY_FILE;
	char *answer_csv_filename = ANSWER_FILE;
	int num_data, num_queries;
	
	fprintf(stderr, "PJT_DIM=%d\n", PJT_DIM);
	struct_dataset *ds_query = read_dataset_n(1, &query_ftr_filename);
	num_queries = ds_query->num_data;
	fprintf(stderr, "read query file OK. the number of queries = %d\n", num_queries);
	query_type *qr = (query_type *)malloc(sizeof(query_type) * num_queries);
	for(int i = 0; i < num_queries; i++) {
		qr[i] = (query_type) { i, ds_query->ftr_id[i].ftr };
	}

	answer_type *correct_answer = read_correct_answer(answer_csv_filename, num_queries);
	fprintf(stderr, "read correct answer OK.\n");

	#if defined(PARTITION_TYPE_QBP)
	pivot_type *pivot = new_pivot(QBP);
	#elif defined(PARTITION_TYPE_PQBP)
	pivot_type *pivot = new_pivot(PQBP);
	#endif
	read_pivot(pivot_file, pivot);
	fprintf(stderr, "read pivot OK\n");

	struct_bucket *bucket = read_compact_bucket(bucket_filename);
	num_data = bucket->num_data;
	fprintf(stderr, "read compact bucket OK. num_data = %d\n", num_data);

	struct_query_sketch *qs = (struct_query_sketch *)malloc(sizeof(struct_query_sketch) * num_queries);
	answer_type *ans = (answer_type *)malloc(sizeof(answer_type) * num_queries);
	#if PRIORITY == 0
	make_bitcnt_tbl(8);
	#endif
	#pragma omp parallel for
	for(int q = 0; q < num_queries; q++) {
		set_query_sketch(&qs[q], &qr[q], pivot);
		#if PRIORITY == 0
			ans[q].dist = hamming(bucket->sk[correct_answer[q].data_num], qs[q].sketch); // 正解データの hamming
		#else
			ans[q].dist = priority(bucket->sk[correct_answer[q].data_num], &qs[q]); // 正解データの priority
		#endif
//		if(q < 10) {
//			printf("q = %d, correct_answer = %d, priority = %u\n", q, correct_answer[q].data_num, ans[q].dist);
//		}
	}

	#pragma omp parallel for
	for(int q = 0; q < num_queries; q++) {
		// 質問のスケッチの順位 = 正解のpriorityより小さいpriorityを持つデータ数 + 1 を求める
		int k, e;
		dist_type p;
		for(int j = k = e = 0; j < num_data; j++) {
			#if PRIORITY == 0
				if((p = hamming(bucket->sk[j], qs[q].sketch)) < ans[q].dist) k++;
			#else
				if((p = priority(bucket->sk[j], &qs[q])) < ans[q].dist) k++;
			#endif
			if(p == ans[q].dist) e++;
		}
//		ans[q].dist = k + e / 2;
		ans[q].dist = k; // Hamming のときは，この方が正確（理由は未考察）
//		if(q < 10) {
//			printf("q = %d, pri = %d\n", q, ans[q].dist);
//		}
	}

	qsort(ans, num_queries, sizeof(answer_type), comp_answer);

	int multi_K[1000];
	for(int i = 0; i < 1000; i++) {
		multi_K[i] = (int)(ans[(int)(num_queries * i / 1000)].dist);
	}

	for(int i = 700; i <= 950; i += 50) {
		int K = multi_K[i]; 
		printf("K for recall, %d, %1.4lf, w , %d, pivot, %s\n", i / 10, (double)K / num_data * 100, PJT_DIM, pivot_file);
	}
	for(int i = 960; i <= 990; i += 10) {
		int K = multi_K[i]; 
		printf("K for recall, %d, %1.4lf, w , %d, pivot, %s\n", i / 10, (double)K / num_data * 100, PJT_DIM, pivot_file);
	}

	return 0;
}