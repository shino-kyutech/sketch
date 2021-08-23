// 全探索（sequential search)によるk近傍検索をするプログラム
// fseek しない＋まとめ読みする版
// 外側ループ＝データ
// 内側ループ＝質問
// データごとに複数の質問の検索処理をすることによって，質問あたりのデータ読込コストを低減する．
// ただし，この方法では，個々の質問に対する応答性を改善するわけではない．
// あくまでも，比較のための実験で，実用性は低い．
// 
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include "config.h"
#include "parm.h" // 実際には実行時に定義して用いるパラメータ
//#define ANSWER_WITH_DATA_ID
#include "ftr.h"
#include "kNN_search.h"
#include <omp.h>

int main(int argc, char *argv[])
{
	int num_ftr_files = argc - 1;
	int num_data;
	char **ds_ftr_filename = argv + 1;
	file_handle ds_fh[num_ftr_files];
	ftr_header_type ds_hd[num_ftr_files];
	struct_ftr_id ds_ftr_id;

	int num_queries;
	char *query_ftr_filename = QUERY;
	ftr_header_type q_hd;
	file_handle q_fh;
	struct_ftr_id *query_ftr_id;
	answer_type *ans;
	kNN_buffer **buff;
	
	time_t timer1, timer2;

	for(int i = num_data = 0; i < num_ftr_files; i++) {
		if((ds_fh[i] = open_ftr_file(ds_ftr_filename[i], &ds_hd[i])) == OPEN_ERROR) {
			fprintf(stderr, "cannot open ftr file = %s\n", ds_ftr_filename[i]);
		}
		num_data += ds_hd[i].num_data;
	}
	fprintf(stderr, "open ftr files OK (total number of data = %d)\n", num_data);

	if((q_fh = open_ftr_file(query_ftr_filename, &q_hd)) == OPEN_ERROR) {
		fprintf(stderr, "cannot open query ftr file = %s\n", query_ftr_filename);
		return -2;
	}
	num_queries = q_hd.num_data;
	fprintf(stderr, "open query ftr file = %s OK (number of queries = %d)\n", query_ftr_filename, num_queries);
	query_ftr_id = (struct_ftr_id *)malloc(sizeof(struct_ftr_id) * num_queries);
	ans = (answer_type *)malloc(sizeof(answer_type) * num_queries);
	buff = (kNN_buffer **)malloc(sizeof(kNN_buffer *) * num_queries);
	for(int q = 0; q < num_queries; q++) {
		get_ftr_id(q_fh, &q_hd, q, &query_ftr_id[q]); // 初めに全質問をの ftr （特徴ベクトル）と id を読み込む
		buff[q] = new_kNN_buffer(NUM_K);
	}
	CLOSE(q_fh);
	fprintf(stderr, "read all queries OK\n");

	#ifdef _OPENMP
	omp_set_num_threads(NUM_THREADS);
	fprintf(stderr, "Sequential k-NN search starts by query parallel: k = %d, #query = %d, #data = %d, #thread = %d\n", NUM_K, num_queries, num_data, NUM_THREADS); 
	#else
	fprintf(stderr, "Sequential k-NN search starts without parallel: k = %d, #query = %d, #data = %d\n", NUM_K, num_queries, num_data); 
	#endif

	timer1 = time(NULL);

	int data_num = 0;
	for(int i = 0; i < num_ftr_files; i++) {
		for(int j = 0; j < ds_hd[i].num_data; j++, data_num++) {
			if(data_num % 1000000 == 0) fprintf(stderr, "*");
			get_ftr_id(ds_fh[i], &ds_hd[i], j, &ds_ftr_id);
			#ifdef _OPENMP
			#pragma omp parallel for
			#endif
			for(int q = 0; q < num_queries; q++) {
				ans[q].dist = DISTANCE(query_ftr_id[q].ftr, ds_ftr_id.ftr, q_hd.data_dim);
				if(ans[q].dist <= buff[q]->k_nearest) {
					ans[q].data_id = ds_ftr_id.data_id;
					ans[q].data_num = data_num;
					push_kNN_buffer(&ans[q], buff[q]);
				}
			}
		}
		CLOSE(ds_fh[i]);
	}
	#ifdef _OPENMP
	#pragma omp parallel for
	#endif
	for(int q = 0; q < num_queries; q++) {
		final_flush_kNN_buffer(buff[q]);
	}
	timer2 = time(NULL);
	fprintf(stderr, "\n%.1lf, (sec)\n", difftime(timer2, timer1));
	
	#ifdef PRINTOUT
	printf("q_number, q_id, NN[1]_idx, NN[1]_dist, NN[1]_id, ...\n");
	for(int q = 0; q < num_queries; q++) {
		#if defined(DEEP1B)
		printf("%d,%d", q, query_ftr_id[q].data_id);
		for(int j = 0; j < NUM_K; j++) {
			printf(",%d,%d,%d", buff[q]->buff[j].data_num, buff[q]->buff[j].dist, buff[q]->buff[j].data_id);
		}
		#elif defined(DECAF)
		printf("%d,%ld", q, query_ftr_id[q].data_id);
		for(int j = 0; j < NUM_K; j++) {
			printf(",%d,%d,%ld", buff[q]->buff[j].data_num, buff[q]->buff[j].dist, buff[q]->buff[j].data_id);
		}
		#endif
		printf("\n");
	}
	#endif

	return 0;
}
