// スケッチを用いない直接の検索
// sequential access や random read，まとめ読みの有無での走査速度を測定する

#include <stdio.h>
#include <string.h>
#include "config.h"
#include "ftr.h"
#include "kNN_search.h"
//#include "sketch.h"
//#include "quick.h"
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/resource.h>


int main(int argc, char *argv[])
{
	int num_ftr_files = argc - 1;
	char **dataset_ftr_filename = argv + 1;
	char *query_ftr_filename = QUERY_FILE;
	int num_data, num_queries;
	
	struct_dataset *ds_query = read_dataset_n(1, &query_ftr_filename);
	num_queries = ds_query->num_data;
	fprintf(stderr, "read query file OK. the number of queries = %d\n", num_queries);
	query_type *qr = (query_type *)malloc(sizeof(query_type) * num_queries);
	for(int i = 0; i < num_queries; i++) {
		qr[i] = (query_type) {i, ds_query->ftr_id[i].ftr}; 
	}

	dataset_handle dh;
	#if defined(_OPENMP) && defined(NUM_THREADS)
	dh.num_threads = NUM_THREADS;
	#else
	dh.num_threads = 1;
	#endif
	
	dh.sorted = 0;

	#if defined(FTR_ON_MAIN_MEMORY)
		dh.ftr_on = MAIN_MEMORY;
		dh.ds = read_dataset_n(num_ftr_files, dataset_ftr_filename);
		dh.mf = NULL;
		num_data = dh.ds->num_data;
	fprintf(stderr, "read dataset file(s) OK. total number of data = %d\n", num_data);
	#elif defined(FTR_ON_SECONDARY_MEMORY)
		dh.ftr_on = SECONDARY_MEMORY;
		dh.mf = (struct_multi_ftr **)malloc(sizeof(struct_multi_ftr *) * dh.num_threads);
		for(int t = 0; t < dh.num_threads; t++) {
			dh.mf[t] = open_multi_ftr(num_ftr_files, dataset_ftr_filename, BLOCK_SIZE);
		}
		dh.ds = NULL;
		num_data = dh.mf[0]->num_data;
		fprintf(stderr, "open dataset file(s) OK. total number of data = %d\n", num_data);
	#else
		#error "FTR_ON_MAIN_MEMORY or FTR_ON_SECONDARY_MEMORY should be defined."
	#endif // FTR_ON

//  検索をする
	int nt = 1;
	#ifdef _OPENMP
	#if defined(NUM_THREADS) && NUM_THREADS > 1
	nt = NUM_THREADS;
	omp_set_num_threads(NUM_THREADS);
	fprintf(stderr, "omp_set_num_threads OK, nt = %d, omp_get_max_threads = %d\n", nt, omp_get_max_threads());
	#endif
	#endif

	num_queries = NUM_Q;
	int k = NUM_D; // 検索対象のデータ数
	if(k * num_queries > num_data) {
		fprintf(stderr, "NUM_Q * NUM_D = %d exceeds the number of data in ftr file(s) = %d\n", NUM_Q * NUM_D, num_data);
		exit(0);
	}
	srandom(SEED);
	int num_top_k = NUM_K; // kNN 検索で求める解の個数
//	int *data_num = (int *)malloc(sizeof(int) * NUM_Q * NUM_D);
	int **data_num_seq, **data_num_rand;
	if(strcmp(ACCESS_MODE, "BOTH") == 0 || strcmp(ACCESS_MODE, "SEQUENTIAL") == 0) {
		data_num_seq = (int **)malloc(sizeof(int *) * NUM_Q);
		int st = random() % num_data;
		for(int j = 0; j < NUM_Q; j++) {
			data_num_seq[j] = (int *)malloc(sizeof(int) * NUM_D);
			if(st + k >= num_data) st = st + k - num_data;
			for(int i = 0; i < k; i++) {
				data_num_seq[j][i] = st++;
			}
		}
	}
	if(strcmp(ACCESS_MODE, "BOTH") == 0 || strcmp(ACCESS_MODE, "RANDOM") == 0) {
		data_num_rand = (int **)malloc(sizeof(int *) * NUM_Q);
		for(int j = 0; j < NUM_Q; j++) {
			data_num_rand[j] = (int *)malloc(sizeof(int) * NUM_D);
			for(int i = 0; i < k; i++) {
				data_num_rand[j][i] = random() % num_data;
			}
		}
	}
	kNN_buffer **top_k = (kNN_buffer **)malloc(sizeof(kNN_buffer *) * num_queries);
	char *amode[] = { "SEQUENTIAL", "RAMDOM" };
	int nm = 2;
	if(strcmp(ACCESS_MODE, "BOTH") != 0) {
		nm = 1;
		amode[0] = ACCESS_MODE;
	}
	for(int m = 0; m < nm; m++) {
		char *mode = amode[m];
		int **data_num;
		if(strcmp(mode, "SEQUENTIAL") == 0) {
			data_num = data_num_seq;
		} else {
			data_num = data_num_rand;
		}
		// NUM_CANDIDATES = データセットに対する割合（パーミリアド（万分率））
		struct timespec tp1, tp2;
		clock_gettime(CLOCK_REALTIME, &tp1);

		for(int i = 0; i < num_queries; i++) {
			top_k[i] = new_kNN_buffer(num_top_k);
			if(num_top_k == 1) {
				search_NN(&dh, &qr[i], k, data_num[i], top_k[i]);
			} else {
				search_kNN(&dh, &qr[i], k, data_num[i], top_k[i]);
			}
			if(num_queries <= 20) {
				fprintf(stderr, "query[%d] = %d, %d, %d\n", i, data_num[i][0], top_k[i]->buff[0].data_num, top_k[i]->buff[0].dist);
			}
		}

		clock_gettime(CLOCK_REALTIME,&tp2);
		long sec = tp2.tv_sec - tp1.tv_sec;
		long nsec = tp2.tv_nsec - tp1.tv_nsec;
		if(nsec < 0){
			sec--;
			nsec += 1000000000L;
		}
		printf("%ld.%09ld, %s, block_size =, %d, #threads =, %d, #n, %d, #q, %d, %s, file =, %s\n", sec, nsec, mode, BLOCK_SIZE, nt, NUM_D, NUM_Q, dh.ftr_on == MAIN_MEMORY ? "RAM" : "2ND", dataset_ftr_filename[0]);
	}

	return 0;
}
