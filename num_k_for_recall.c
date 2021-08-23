#include <stdio.h>
#include <string.h>
#include "parm.h"
#include "bit_op.h"
#include "config.h"
#include "ftr.h"
#include "kNN_search.h"
#include "sketch.h"
#include "e_time.h"

// バケット（bkt）を用いて，recallのために必要な候補数k'のデータ数に対する割合（k'/n）を求める
int main(int argc, char *argv[])
{
	if(argc != 4) {
		fprintf(stderr, "usage> %s p_start p_end p_step\n", argv[0]);
		return -1;
	}
	struct timespec tp1, tp2;
	clock_gettime(CLOCK_REALTIME, &tp1);
	double p_start = atof(argv[1]);
	double p_end = atof(argv[2]);
	double p_step = atof(argv[3]);
	int num_p = (p_end - p_start + 0.005) / p_step + 1;
	double pl[num_p];
	for(int i = 0; i < num_p; i++) {
		pl[i] = p_start + p_step * i;
	}
	char *pivot_file = PIVOT_FILE;
	char *bucket_filename = BUCKET_FILE;
	char *query_ftr_filename = QUERY_FILE;
	char *answer_csv_filename = ANSWER_FILE;
	int num_data, num_queries;

	fprintf(stderr, "PJT_DIM=%d\n", PJT_DIM);
	#ifndef NARROW_SKETCH
	fprintf(stderr, "this program supports only narrow sketches: w <= 32\n");
	return -1;
	#endif
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
	use_system("VmSize");

	struct_query_sketch *qs[num_p];
	answer_type *ans[num_p];
	for(int i = 0; i < num_p; i++) {
		qs[i] = (struct_query_sketch *)malloc(sizeof(struct_query_sketch) * num_queries);
		ans[i] = (answer_type *)malloc(sizeof(answer_type) * num_queries);
	}

	struct_bucket *bucket = read_bucket(bucket_filename);
	num_data = bucket->num_data;
	fprintf(stderr, "read compact bucket OK. num_data = %d\n", num_data);
	int *bkt = bucket->bkt, *idx = bucket->idx;

	#pragma omp parallel for
	for(int q = 0; q < num_queries; q++) {
		for(int i = 0; i < num_p; i++) {
			set_query_sketch_p(&qs[i][q], &qr[q], pivot, pl[i]);
			ans[i][q].dist = priority(bucket->sk[correct_answer[q].data_num], &qs[i][q]); // 正解データの priority
		}
	}

	struct_que_c2_n *que_pool = NULL;
	#ifdef NUM_THREADS
	omp_set_num_threads(NUM_THREADS);
	#endif
	int nt = omp_get_max_threads();
	fprintf(stderr, "Malloc for que requests (%ld * %d = %ld is required).\n", sizeof(struct_que_c2_n), nt, sizeof(struct_que_c2_n) * nt);
	que_pool = (struct_que_c2_n *)malloc(sizeof(struct_que_c2_n) * nt);
	if(que_pool == NULL) {
		fprintf(stderr, "Malloc for que failed\n");
		exit(0);
	}
	use_system("VmSize");

	#pragma omp parallel for
	for(int q = 0; q < num_queries; q++) {
		sketch_type s;
		QUE_c2 qu, qu2;
		struct_que_c2_n *que = &que_pool[omp_get_thread_num()];

		for(int i = 0; i < num_p; i++) {
			int *bd = qs[i][q].bd, *bd_idx = qs[i][q].idx;
			s = qs[i][q].sketch;
			int k = 0;
			for(int j = bkt[s]; j < bkt[s + 1]; j++, k++) {
				if(idx[j] == correct_answer[q].data_num) {
					ans[i][q].data_num = q + 1;
					ans[i][q].dist = k + 1;
					goto END_SEARCH; // 検索終了
				}
			}

			s = s ^ (1 <<  bd_idx[0]);
			for(int j = bkt[s]; j < bkt[s + 1]; j++, k++) {
				if(idx[j] == correct_answer[q].data_num) {
					ans[i][q].data_num = q + 1;
					ans[i][q].dist = k + 1;
					goto END_SEARCH; // 検索終了
				}
			}

			make_empty_que_c2_n(que);

			// enq pattern of 0...10
			qu.cursor = new_que_e2_n(que);
			qu.key = bd[bd_idx[1]];
			que->details[qu.cursor].sk = qs[i][q].sketch ^ (1 << bd_idx[1]);
			que->details[qu.cursor].pt = 1 << 1; // pt = "0...00000010"
			enq_c2_n(&qu, que);		

			while(deq_c2_n(&qu, que)) {
				s = que->details[qu.cursor].sk;
				for(int j = bkt[s]; j < bkt[s + 1]; j++, k++) {
					if(idx[j] == correct_answer[q].data_num) {
						ans[i][q].data_num = q + 1;
						ans[i][q].dist = k + 1;
						goto END_SEARCH; // 検索終了
					}
				}

				switch(que->details[qu.cursor].pt & 15) {
					int m;
				case 0: // X0000 -> enq(X0001) and enq(Y10^{m+1}) if X0000 = Y010^m
				case 8: // X1000 -> enq(X1001) and enq(Y10^{m+1}) if X0000 = Y010^m
					m = lsb_pos(que->details[qu.cursor].pt);
					if(m > 0 && m < PJT_DIM - 1 && !(que->details[qu.cursor].pt & (1 << (m + 1)))) {
						// Y010^m -> Y10^{m+1}
						qu2.cursor = new_que_e2_n(que);
						qu2.key = qu.key + bd[bd_idx[m + 1]] - bd[bd_idx[m]];
						que->details[qu2.cursor].sk = (que->details[qu.cursor].sk ^ (1 << bd_idx[m + 1])) ^ (1 << bd_idx[m]);
						que->details[qu2.cursor].pt = que->details[qu.cursor].pt + (1 << m);
						// Y010^m -> Y010^{m-1}1
						qu.key = qu.key + bd[bd_idx[0]];
						que->details[qu.cursor].sk = que->details[qu.cursor].sk ^ (1 << bd_idx[0]);
						que->details[qu.cursor].pt = que->details[qu.cursor].pt + 1;
						enq_c2_n(&qu, que);
						enq_c2_n(&qu2, que);
					} else {
						qu.key = qu.key + bd[bd_idx[0]];
						que->details[qu.cursor].sk = que->details[qu.cursor].sk ^ (1 << bd_idx[0]);
						que->details[qu.cursor].pt = que->details[qu.cursor].pt + 1;
						enq_c2_n(&qu, que);
					}
					break;
				case 4:  // X0100 -> enq(X0101) and enq(X1000)
					// X1000
					qu2.cursor = new_que_e2_n(que);
					qu2.key = qu.key + bd[bd_idx[3]] - bd[bd_idx[2]];
					que->details[qu2.cursor].sk = (que->details[qu.cursor].sk ^ (1 << bd_idx[3])) ^ (1 << bd_idx[2]);
					que->details[qu2.cursor].pt = que->details[qu.cursor].pt + 4;
					// X0101
					qu.key = qu.key + bd[bd_idx[0]];
					que->details[qu.cursor].sk = que->details[qu.cursor].sk ^ (1 << bd_idx[0]);
					que->details[qu.cursor].pt = que->details[qu.cursor].pt + 1;
					enq_c2_n(&qu, que);
					enq_c2_n(&qu2, que);
					break;
				case 1:  // X0001 -> enq(X0010)
				case 5:  // X0101 -> enq(X0110)
				case 9:  // X1001 -> enq(X1010)
				case 13: // X1101 -> enq(X1110) (note that X <> 0, because 0...00 and 0...01 is already processed before while loop)
					qu.key = qu.key + bd[bd_idx[1]] - bd[bd_idx[0]];
					que->details[qu.cursor].sk = (que->details[qu.cursor].sk ^ (1 << bd_idx[1])) ^ (1 << bd_idx[0]);
					que->details[qu.cursor].pt = que->details[qu.cursor].pt + 1;
					enq_c2_n(&qu, que);
					break;
				case 2:  // X0010 -> enq(X0011) and enq(X0100)
				case 10: // X1010 -> enq(X1011) and enq(X1100)
					// X0100 and X1100
					qu2.cursor = new_que_e2_n(que);
					qu2.key = qu.key +  bd[bd_idx[2]] -  bd[bd_idx[1]];
					que->details[qu2.cursor].sk = (que->details[qu.cursor].sk ^ (1 << bd_idx[2])) ^ (1 << bd_idx[1]);
					que->details[qu2.cursor].pt = que->details[qu.cursor].pt + 2;
					// X0011 and X1011
					qu.key = qu.key + bd[bd_idx[0]];
					que->details[qu.cursor].sk = que->details[qu.cursor].sk ^ (1 <<  bd_idx[0]);
					que->details[qu.cursor].pt = que->details[qu.cursor].pt + 1;
					enq_c2_n(&qu, que);
					enq_c2_n(&qu2, que);
					break;
				case 6:  // X0110 -> enq(X0111)
				case 12: // X1100 -> enq(X1101)
				case 14: // X1110 -> enq(10111)
					qu.key = qu.key + bd[bd_idx[0]];
					que->details[qu.cursor].sk = que->details[qu.cursor].sk ^ (1 <<  bd_idx[0]);
					que->details[qu.cursor].pt = que->details[qu.cursor].pt + 1;
					enq_c2_n(&qu, que);
					break;
				case 3:  // X0011
				case 7:  // X0111
				case 11: // X1011
				case 15: // X1111 -> nothing to do
					break;
				}
			}
			END_SEARCH:	
			if(q < 10) {
				printf("q = %d, key = %d, k = %d\n", q, qu.key, ans[i][q].dist);
			}
			;
		}
	}

	for(int i = 0; i < num_p; i++) {
		qsort(ans[i], num_queries, sizeof(answer_type), comp_answer);
	}

	int multi_K[num_p][1000];
	for(int i = 0; i < 1000; i++) {
		for(int j = 0; j < num_p; j++) {
			multi_K[j][i] = (int)(ans[j][(int)(num_queries * i / 1000)].dist);
		}
	}

	clock_gettime(CLOCK_REALTIME, &tp2);
	printf("%.2lf sec\n", e_time(&tp1, &tp2));

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