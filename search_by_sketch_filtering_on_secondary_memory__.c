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

static int comp_data_num(const void *a, const void *b) {
	if(*((int *) a) < *((int *) b))
		return -1;
	else if(*((int *) a) == *((int *) b))
		return 0;
	else
		return 1;
}

int main(int argc, char *argv[])
{
	#if NUM_K > 0
	int num_ftr_files = argc - 1;
	char **dataset_ftr_filename = argv + 1;
	#endif
	char *pivot_file = PIVOT_FILE;
	char *bucket_filename = BUCKET_FILE;
	char *query_ftr_filename = QUERY_FILE;
	#if NUM_K > 0
	char *answer_csv_filename = ANSWER_FILE;
	char *result_filename = RESULT_FILE;
	char temp_str[1000];
	char *result_filename2 = temp_str;
	#endif
	int num_data, num_queries;

	fprintf(stderr, "PJT_DIM=%d\n", PJT_DIM);
	struct_dataset *ds_query = read_dataset_n(1, &query_ftr_filename);
	num_queries = ds_query->num_data;
	fprintf(stderr, "read query file OK. the number of queries = %d\n", num_queries);
	query_type *qr = (query_type *)malloc(sizeof(query_type) * num_queries);
	for(int i = 0; i < num_queries; i++) {
		qr[i] = (query_type) { i, ds_query->ftr_id[i].ftr}; // , ds_query->ftr_id[i].data_id ;
	}

	#if NUM_K > 0
	answer_type *correct_answer = read_correct_answer(answer_csv_filename, num_queries);
	fprintf(stderr, "read correct answer OK.\n");
	#endif

	#if defined(PARTITION_TYPE_QBP)
	pivot_type *pivot = new_pivot(QBP);
	#elif defined(PARTITION_TYPE_PQBP)
	pivot_type *pivot = new_pivot(PQBP);
	#endif
	read_pivot(pivot_file, pivot);
	fprintf(stderr, "read pivot OK\n");

	#if defined(FILTERING_BY_SKETCH_ENUMERATION) || defined(FILTERING_BY_SKETCH_ENUMERATION_C2N)
	struct_bucket *bucket_ds = read_bucket(bucket_filename);
	fprintf(stderr, "read bucket OK, ");
	#else
	struct_bucket *bucket_ds = read_compact_bucket(bucket_filename); // SEQUENTIAL_FILTERING or SEQUENTIAL_FILTERING_USING_BUCKET or SEQUENTIAL_FILTERING_USING_HAMMING
	fprintf(stderr, "read compact bucket OK, ");
	#endif
	num_data = bucket_ds->num_data;
	fprintf(stderr, "number of data = %d\n", num_data);

#if NUM_K <= 0
	// NUM_K = 0 -> filtering only, -1 -> scoreing only 
#else
	int num_top_k = NUM_K;

	dataset_handle dh;
//	dh.num_ftr_files = num_ftr_files;
//	dh.filename = dataset_ftr_filename;
	#if defined(_OPENMP) && defined(NUM_THREADS)
	dh.num_threads = NUM_THREADS;
	#else
	dh.num_threads = 1;
	#endif
	
	#ifdef FTR_SORT_BY_SKETCH_IN_ADVANCE
	fprintf(stderr, "FTR is sorted in advanve\n");
	dh.sorted = 1;
	#else
	fprintf(stderr, "FTR is NOT sorted. Arrangement is as is.\n");
	dh.sorted = 0;
	#endif

	#if defined(FTR_ON_MAIN_MEMORY)
		dh.ftr_on = MAIN_MEMORY;
		dh.ds = read_dataset_n(num_ftr_files, dataset_ftr_filename);
		dh.mf = NULL;
		fprintf(stderr, "read dataset file(s) OK. total number of data = %d\n", dh.ds->num_data);
		if(num_data != dh.ds->num_data) {
			fprintf(stderr, "bucket (filename = %s) is not compatible with datasets (filename = %s, ... )\n", bucket_filename, dataset_ftr_filename[0]);
			return -1;
		}
		#if defined(FTR_SORT_BY_SKETCH) || defined(SEQUENTIAL_FILTERING_USING_BUCKET) || defined(SEQUENTIAL_FILTERING_USING_HAMMING) || defined(FILTERING_BY_SKETCH_ENUMERATION) || defined(FILTERING_BY_SKETCH_ENUMERATION_C2N)
		fprintf(stderr, "sort dataset in sketch order ... ");
		sort_dataset(dh.ds, bucket_ds->idx);
		dh.sorted = 1;
		fprintf(stderr, "OK\n");
		#endif
	#elif defined(FTR_ON_SECONDARY_MEMORY)
		dh.ftr_on = SECONDARY_MEMORY;
		dh.mf = (struct_multi_ftr **)malloc(sizeof(struct_multi_ftr *) * dh.num_threads);
		for(int t = 0; t < dh.num_threads; t++) {
			dh.mf[t] = open_multi_ftr(num_ftr_files, dataset_ftr_filename, BLOCK_SIZE);
		}
		dh.ds = NULL;
		if(num_data != dh.mf[0]->num_data) {
			fprintf(stderr, "bucket (filename = %s, num_data = %d) is not compatible with datasets (filename = %s, ... , num_data = %d)\n", bucket_filename, num_data, dataset_ftr_filename[0], dh.mf[0]->num_data);
			return -1;
		}
	#else
		#error "FTR_ON_MAIN_MEMORY or FTR_ON_SECONDARY_MEMORY should be defined."
	#endif // FTR_ON
	
#endif // NUM_K

//  検索をする
	#ifdef _OPENMP
		#if defined(NUM_THREADS) && NUM_THREADS > 1
		omp_set_num_threads(NUM_THREADS);
		#endif
	#endif

	// NUM_K = kNN 検索で求める解の個数（ただし，-1: scoring のみ，0: filtering のみ）

	struct_query_sketch *query_sketch = (struct_query_sketch *)malloc(sizeof(struct_query_sketch));
	#if defined(FILTERING_BY_SKETCH_ENUMERATION)
		fprintf(stderr, "FILTERING_BY_SKETCH_ENUMERATION\n");
		struct_que *que = (struct_que *)malloc(sizeof(struct_que));
	#elif defined(FILTERING_BY_SKETCH_ENUMERATION_C2N)
		fprintf(stderr, "FILTERING_BY_SKETCH_ENUMERATION_C2N\n");
		struct_que_c2_n *que = (struct_que_c2_n *)malloc(sizeof(struct_que_c2_n));
	#elif defined(SEQUENTIAL_FILTERING)
		fprintf(stderr, "SEQUENTIAL_FILTERING\n");
		dist_type *score = (dist_type *)malloc(sizeof(dist_type) * num_data);
		int *idx = (int *)malloc(sizeof(int) * num_data);
		#if NUM_K > 0
		int *org_idx = NULL;
		if(dh.sorted) {
			org_idx = (int *)malloc(sizeof(int) * num_data);
			for(int i = 0; i < num_data; i++) {
				org_idx[i] = bucket_ds->idx[i];
			}
		}
		#endif
		#ifdef _OPENMP
		#if defined(NUM_THREADS) && NUM_THREADS > 1
		work_select_para *wp = new_work_select_para(NUM_THREADS);
		#endif
		#endif
	#elif defined(SEQUENTIAL_FILTERING_USING_BUCKET)
		fprintf(stderr, "SEQUENTIAL_FILTERING_USING_BUCKET\n");
		int num_nonempty_buckets = bucket_ds->num_nonempty_buckets;
		dist_type *score = (dist_type *)malloc(sizeof(dist_type) * num_nonempty_buckets);
		int *idx = (int *)malloc(sizeof(int) * num_nonempty_buckets);
	#elif defined(SEQUENTIAL_FILTERING_USING_HAMMING)
		fprintf(stderr, "SEQUENTIAL_FILTERING_USING_HAMMING\n");
		int num_nonempty_buckets = bucket_ds->num_nonempty_buckets;
		dist_type *score = (dist_type *)malloc(sizeof(dist_type) * num_nonempty_buckets);
		int *idx = (int *)malloc(sizeof(int) * num_nonempty_buckets);
		make_bitcnt_tbl(PJT_DIM);
		make_bitnum_pat(PJT_DIM);
	#endif

	#if NUM_K >= 0
	int *data_num = (int *)malloc(sizeof(int) * num_data);
	#endif
	#if NUM_K > 0
	kNN_buffer **top_k = (kNN_buffer **)malloc(sizeof(kNN_buffer *) * num_queries);
	#endif
	// NUM_CANDIDATES = データセットに対する割合（ppm（百万分率））
	int nc[6] = {NUM_CANDIDATES, NUM_CANDIDATES1, NUM_CANDIDATES2, NUM_CANDIDATES3, NUM_CANDIDATES4, NUM_CANDIDATES5};
	for(int i = 0; i < 6; i++) {
		if(nc[i] == 0) break;
		int num_candidates = nc[i] * 0.000001 * num_data;
		fprintf(stderr, "num_data = %d, NUM_CANDIDATES = %d, num_candidates = %d\n", num_data, nc[i], num_candidates);
		#if defined(SEQUENTIAL_FILTERING)
		#elif defined(SEQUENTIAL_FILTERING_USING_BUCKET) || defined(SEQUENTIAL_FILTERING_USING_HAMMING)
		int num_buckets_of_candidates = (double)num_nonempty_buckets * nc[i] * 0.0001 * 1.5;
		fprintf(stderr, "number of buckets to select candidates = %d\n", num_buckets_of_candidates);
		#endif
		struct timespec tp1, tp2;
		clock_gettime(CLOCK_REALTIME, &tp1);


		#ifdef NUM_Q
		num_queries = NUM_Q;
		#endif
		for(int i = 0; i < num_queries; i++) {

			struct timespec tp3, tp4;
			clock_gettime(CLOCK_REALTIME, &tp3);

			set_query_sketch(query_sketch, &qr[i], pivot);
		#if defined(FILTERING_BY_SKETCH_ENUMERATION) || defined(FILTERING_BY_SKETCH_ENUMERATION_C2N)
			#ifdef FILTERING_BY_SKETCH_ENUMERATION
			filtering_by_sketch_enumeration(query_sketch, bucket_ds, que, data_num, num_candidates);
			#else
			filtering_by_sketch_enumeration_c2_n(query_sketch, bucket_ds, que, data_num, num_candidates);
			#endif
			#if NUM_K > 0
			int k = num_candidates;
//			fprintf(stderr, "dataset is %s\n", dh.sorted ? "sorted" : "as is");
//			for(int n = 0; n < 20; n++) {
//				fprintf(stderr, "data_num[%d] = %d, idx[data_num[%d]] = %d\n", n, data_num[n], n, bucket_ds->idx[data_num[n]]);
//			}
			if(!dh.sorted) {
				for(int n = 0; n < k; n++) {
					data_num[n] = bucket_ds->idx[data_num[n]];
				}
				qsort(data_num, k, sizeof(int), comp_data_num);
			}
			#endif
		#else
			#if defined(_OPENMP) && defined(NUM_THREADS) && NUM_THREADS > 1 
			// omp_set_num_threads(NUM_THREADS);
			#pragma omp parallel for
			#endif
			#if defined(SEQUENTIAL_FILTERING) 
				for(int j = 0; j < num_data; j++) {
					// idx[j] = j;
					// score[j] = priority(bucket_ds->sk[j], query_sketch);
					#if NUM_K > 0
					idx[j] = dh.sorted ? org_idx[j] : j;
					#else
					idx[j] = j;
					#endif
					score[j] = priority(bucket_ds->sk[idx[j]], query_sketch);
				}
	//			printf("scoring done for Sequential filtering\n");
			#elif defined(SEQUENTIAL_FILTERING_USING_BUCKET)
				for(int j = 0; j < num_nonempty_buckets; j++) {
					idx[j] = j;
					score[j] = priority(bucket_ds->sk_num[j].sk, query_sketch);
				}
			#elif defined(SEQUENTIAL_FILTERING_USING_HAMMING)
				for(int j = 0; j < num_nonempty_buckets; j++) {
					idx[j] = j;
					score[j] = hamming(bucket_ds->sk_num[j].sk, query_sketch->sketch);
				}
			#endif

			#if NUM_K == -1
				// scoring only
			#else
				#if defined(SEQUENTIAL_FILTERING)
					#if NUM_THREADS == 1
						quick_select_k(idx, score, 0, num_data - 1, num_candidates);
						int k;
						for(k = 0; k < num_candidates; k++) {
							data_num[k] = idx[k];
						}
					#else
						int k = quick_select_k_para_work(idx, score, num_data, num_candidates, wp);
						int k2 = get_top_k_from_work_para(data_num, idx, wp);
						if(k != k2) fprintf(stderr, "k = %d, k2 = %d\n", k, k2);
						k = k2;
					#endif
//					qsort(data_num, k, sizeof(int), comp_data_num);
				#elif defined(SEQUENTIAL_FILTERING_USING_BUCKET) || defined(SEQUENTIAL_FILTERING_USING_HAMMING)
					#if NUM_THREADS == 1
						int b = quick_select_bkt(idx, score, bucket_ds->sk_num, 0, num_nonempty_buckets - 1, num_candidates);
					#else
						int b = quick_select_bkt_para(idx, score, bucket_ds->sk_num, num_nonempty_buckets, num_candidates, NUM_THREADS);
					#endif
					int k = 0;
					for(int j = 0; j < b; j++) {
						for(int e = 0; e < bucket_ds->sk_num[idx[j]].num; e++) {
							#if NUM_K > 0
								if(dh.sorted == 0) {
									data_num[k] = bucket_ds->idx[bucket_ds->sk_num[idx[j]].pos + e];
								} else {
									data_num[k] = bucket_ds->sk_num[idx[j]].pos + e;
								}
							#else
								data_num[k] = bucket_ds->sk_num[idx[j]].pos + e;
							#endif
							k++;
						}
					}
					if(k < num_candidates) {
						fprintf(stderr, "too small number of buckets to select candidates: k = %d, nc = %d\n", k, num_candidates);
					}
					if(dh.sorted == 0) {
						qsort(data_num, k, sizeof(int), comp_data_num);
					}
				#endif
			#endif
		#endif // filtering
			#if NUM_K <= 0
				// scoreing or filtering only
			#else
				top_k[i] = new_kNN_buffer(num_top_k);
				if(num_top_k == 1) {
					search_NN(&dh, &qr[i], k, data_num, top_k[i]);
				} else {
					search_kNN(&dh, &qr[i], k, data_num, top_k[i]);
				}
			#endif
			
			clock_gettime(CLOCK_REALTIME,&tp4);
			long sec = tp4.tv_sec - tp3.tv_sec;
			long nsec = tp4.tv_nsec - tp3.tv_nsec;
			if(nsec < 0){
				sec--;
				nsec += 1000000000L;
			}
			#if NUM_K <= 0
			//fprintf(stderr, "%ld.%09ld\n", sec, nsec);
			#else
			#if NUM_Q <= 20
			fprintf(stderr, "%ld.%09ld, %d, %d, %d\n", sec, nsec, i, top_k[i]->buff[0].data_num, top_k[i]->buff[0].dist);
			#endif
//			struct rusage usage;
//			int rc;
//		    rc = getrusage(RUSAGE_SELF, &usage);
//		    if(rc < 0){
//		        printf("Error: getrusage(%d) %s\n", errno, strerror(errno));
//		        return(-1);
//		    }
//			fprintf(stderr, "ru_maxrss = %10ld\n", usage.ru_maxrss);
			#endif
		}

		clock_gettime(CLOCK_REALTIME,&tp2);
		long sec = tp2.tv_sec - tp1.tv_sec;
		long nsec = tp2.tv_nsec - tp1.tv_nsec;
		if(nsec < 0){
			sec--;
			nsec += 1000000000L;
		}
		#if NUM_K > 0
		fprintf(stderr, "%ld.%09ld\n", sec, nsec);
		#else
		#if NUM_K < 0
		printf("%ld.%09ld, %d, %d, scoring, ", sec, nsec, PJT_DIM, NUM_THREADS);
		#else
		printf("%ld.%09ld, %d, %d, filtering, ", sec, nsec, PJT_DIM, NUM_THREADS);
		#endif
		#if defined(SEQUENTIAL_FILTERING) 
		printf("SEQUENTIAL_FILTERING\n");
		#elif defined(SEQUENTIAL_FILTERING_USING_BUCKET)
		printf("USING_BUCKET\n");
		#else
		printf("FILTERING BY ENUMERATION\n");
		#endif
		#endif
		#if NUM_K > 0  // 0 or -1 -> filtering or scoring only
		out_result(result_filename, num_queries, correct_answer, top_k);
		strcpy(result_filename2, result_filename);
		result_filename2 = strtok(result_filename2, ".");
		result_filename2 = strcat(result_filename2, "_result.csv");
		out_result2(result_filename2, PJT_DIM, FTR_DIM, sec, nsec, nc[i], num_queries, correct_answer, top_k, dh.sorted);
		#endif
	}
	return 0;
}
