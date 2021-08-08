// basic definitions for k-NN search (Metric space, query_type and answer_type, kNN_buffer to select top-k)
// Without using sketch
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _OPENMP
#include <omp.h>
#endif
#include "config.h"
#include "kNN_search.h"

// distance functions

dist_type dist_L1(ftr_type a, ftr_type b, int dim)
{
	int j;
	dist_type s = 0;
	
	for(j = 0; j < dim; j++)  {
		s += abs((int)a[j] - (int)b[j]);
	}
	return s;
}

dist_type dist_L1_2(ftr_type a, ftr_type b, int dim, dist_type dist)
{
	int j;
	dist_type s = 0;
	
	for(j = 0; j < dim; j++) {
		s += abs((int)a[j] - (int)b[j]);
		if(s >= dist) return s;
	}
	return s;
}

dist_type dist_L2(ftr_type a, ftr_type b, int dim)
// 注意：平方根を取っていない
{
	int j;
	dist_type s = 0;
	
	for(j = 0; j < dim; j++)  {
		s += ((int)a[j] - (int)b[j]) * ((int)a[j] - (int)b[j]);
	}
	return s;
}

dist_type dist_L2_2(ftr_type a, ftr_type b, int dim, dist_type dist)
{
	int j;
	dist_type s = 0;
	
	for(j = 0; j < dim; j++)  {
		s += ((int)a[j] - (int)b[j]) * ((int)a[j] - (int)b[j]);
		if(s >= dist) return s;
	}
	return s;
}

int point_a[FTR_DIM];				// 同じ点との距離を繰り返し計算するときに点データをコピーしておくための配列．

void set_dist_L2_22(ftr_type a)
{
	int j;
	for(j = 0; j < FTR_DIM; j++) 
		point_a[j] = a[j];
}

dist_type dist_L1_22(ftr_type b)
{
	int j;
	dist_type s = 0;
	for(j = 0; j < FTR_DIM; j++)  {
		s += abs(point_a[j] - (int)b[j]);
	}
	return s;
}

dist_type dist_L2_22(ftr_type b)
{
	int j;
	dist_type s = 0;
	for(j = 0; j < FTR_DIM; j++)  {
		s += (point_a[j] - (int)b[j]) * (point_a[j] - (int)b[j]);
	}
	return s;
}

#ifdef PARTITION_TYPE_PQBP
dist_type part_dist_L2(ftr_type a, ftr_type b, int dim_start, int dim)
// 注意：平方根を取っていない
{
	int j;
	dist_type s = 0;
	
	for(j = dim_start; j < dim_start + dim; j++)  {
		s += ((int)a[j] - (int)b[j]) * ((int)a[j] - (int)b[j]);
	}
	return s;
}

dist_type part_dist_L2_2(ftr_type a, ftr_type b, int dim_start, int dim, dist_type dist)
{
	int j;
	dist_type s = 0;
	
	for(j = dim_start; j < dim_start + dim; j++)  {
		s += ((int)a[j] - (int)b[j]) * ((int)a[j] - (int)b[j]);
		if(s >= dist) return s;
	}
	return s;
}

void set_part_dist_L2_22(ftr_type a, int dim_start, int dim)
{
	int j;
	for(j = dim_start; j < dim_start + dim; j++) {
		point_a[j] = a[j];
	}
}

dist_type part_dist_L1_22(ftr_type b, int dim_start, int dim)
{
	int j;
	dist_type s = 0;
	for(j = dim_start; j < dim_start + dim; j++) {
		s += abs(point_a[j] - (int)b[j]);
	}
	return s;
}

dist_type part_dist_L2_22(ftr_type b, int dim_start, int dim)
{
	int j;
	dist_type s = 0;
	for(j = dim_start; j < dim_start + dim; j++) {
		s += (point_a[j] - (int)b[j]) * (point_a[j] - (int)b[j]);
	}
	return s;
}

#endif
// answer_typeの配列をソートする．
// quick sort のための 関数群 find_pivot, partition_by _pivot, quick_sort

int find_pivot_answer(answer_type ans[], int i, int j)

{
// 初めに 5 回だけピボット候補をランダムに選ぶ．
// i 番目のデータ ans[i].dist と k（ただし，k = i + 1, ... , j）番目のデータ ans[k].dist を比較し，
// すべて等しいときには -1 を返し, 
// そうでないときには, ans[i].dist と異なる ans[k].dist  で最初に 
// 現れたもののうちで, 大きい方の位置(i または k) を返す. 
	int k;

	for(int t = 0; t < 5; t++) {
		k = random() % (j - i + 1) + i;
		if(ans[i].dist != ans[k].dist) {
			return (ans[i].dist > ans[k].dist ? i : k);
		}
	}
	for(k = i + 1; k <= j; k++) {
		if(ans[i].dist != ans[k].dist) {
			return (ans[i].dist > ans[k].dist ? i : k);
		}
	}
	return -1;
}

int partition_by_pivot_answer(answer_type ans[], int i, int j, dist_type piv)
// ans[i], ... , ans[j] をそれらの dist と piv との大小によって分け， 
// piv より小さいものが ans[i], ... , ans[k-1] に，   
// そうでないものが ans[k], ... , ans[j] に来るようにする. 
// 右側のリストの先頭の位置(k)を返す. 
{
   int left, right;
   answer_type temp;
   left = i;   right = j;
   do {
      while(ans[left].dist < piv) left++;
      while(ans[right].dist >= piv) right--;
      if (left < right) { 
         temp = ans[left];
         ans[left] = ans[right];
         ans[right] = temp;
      }
   } while(left <= right);
   return left;
}

void quick_sort_answer(answer_type ans[], int i, int j)
{
	int pivotindex, k;
	pivotindex = find_pivot_answer(ans, i, j);
	if (pivotindex >= 0) {
		k = partition_by_pivot_answer(ans, i, j, ans[pivotindex].dist);
		quick_sort_answer(ans, i, k - 1);
		quick_sort_answer(ans, k, j);
	}
}

void quick_select_k_r_answer(answer_type ans[], int i, int j, int k)
{
	int pivotindex, m;
	pivotindex = find_pivot_answer(ans, i, j);
	if (pivotindex >= 0) {
		m = partition_by_pivot_answer(ans, i, j, ans[pivotindex].dist);
		if(k < m) {
			quick_select_k_r_answer(ans, i, m - 1, k);
		} else if(k > m) {
			quick_select_k_r_answer(ans, m, j, k);
		}
	}
}

void quick_select_k_answer(answer_type ans[], int i, int j, int k)
{
	int pivotindex, m;
//	printf("quick select: i = %d, j = %d, k = %d\n", i, j, k);
	while((pivotindex = find_pivot_answer(ans, i, j)) >= 0) {
//		printf("pivot index = %d, dist[i] = %d, dist[pivotomdex] = %d\n", pivotindex, ans[i].dist, ans[pivotindex].dist);
		m = partition_by_pivot_answer(ans, i, j, ans[pivotindex].dist);
//		printf("m = %d\n", m);
		if(k < m) {
			j = m - 1;
		} else if(k > m) {
			i = m;
		} else {
			break;
		}
//		printf("(in) quick select: i = %d, j = %d, k = %d\n", i, j, k);
	}
//	printf("select DONE\n");
}

kNN_buffer *new_kNN_buffer(int k)
{
	kNN_buffer *b = (kNN_buffer *)malloc(sizeof(kNN_buffer));
	
	b->k = k;
	b->buff = (answer_type *)malloc(sizeof(answer_type) * 2 * k); 
	if(b == NULL) {return b;}
	b->num = 0;
	b->k_nearest = INT_MAX;
	return b;
}

void free_kNN_buffer(kNN_buffer *b)
{
	free(b->buff);
	free(b);
	b = NULL;
}

dist_type make_empty_kNN_buffer(kNN_buffer *b)
{
	b->num = 0;
	b->k_nearest = INT_MAX;
	return b->k_nearest;
}

dist_type push_kNN_buffer(answer_type *a, kNN_buffer *b)
{
	dist_type d;
	if(b->num >= 2 * b->k) {
		d = flush_kNN_buffer(b);
	} else {
		d = b->k_nearest;
	}
	if(a->dist > d) {return d;} // k_nearest より遠い解はバッファーには入れない
	b->buff[b->num] = *a;
	b->num++;
	return d;
}

int comp_answer(const void *a, const void *b) {
	if(((answer_type *) a) -> dist < (((answer_type *) b) -> dist))
		return -1;
	else if(((answer_type *) a) -> dist == (((answer_type *) b) -> dist))
		return 0;
	else
		return 1;
}

dist_type flush_kNN_buffer(kNN_buffer *b)
{
//	qsort(b->buff, b->num, sizeof(answer_type), comp_answer);
//	fprintf(stderr, "flush buffer start, num = %d, k = %d, k_nearest = %d\n", b->num, b->k, b->k_nearest);
	quick_select_k_answer(b->buff, 0, b->num - 1, b->k);
	if(b->num > b->k) {
		b->num = b->k;
	}
	b->k_nearest = b->buff[b->num - 1].dist;
//	fprintf(stderr, "flush done, k_nearest = %d\n", b->k_nearest);
	return b->k_nearest;
}

dist_type final_flush_kNN_buffer(kNN_buffer *b)
{
	qsort(b->buff, b->num, sizeof(answer_type), comp_answer);
//	fprintf(stderr, "flush buffer start, num = %d, k = %d, k_nearest = %d\n", b->num, b->k, b->k_nearest);
//	quick_select_k_answer(b->buff, 0, b->num - 1, b->k);
	if(b->num > b->k) {
		b->num = b->k;
	}
	b->k_nearest = b->buff[b->num - 1].dist;
//	fprintf(stderr, "flush done, k_nearest = %d\n", b->k_nearest);
	return b->k_nearest;
}

dist_type merge_kNN_buffer(kNN_buffer *b, kNN_buffer *pool[], int n)
{
	int i, k = b->k, m;
	answer_type *ans = (answer_type *)malloc(sizeof(answer_type) * k * n);
	
	for(i = m = 0; m < n; m++) {
		for(int j = 0; j < (pool[m]->num < pool[m]->k ? pool[m]->num : pool[m]->k); j++) {
			ans[i++] = pool[m]->buff[j];
		}
	}
	quick_select_k_answer(ans, 0, i - 1, k);
	quick_sort_answer(ans, 0, k - 1);
	for(i = 0; i < k; i++) {
		b->buff[i] = ans[i];
	}
	b->k_nearest = b->buff[k - 1].dist;
	return b->k_nearest;
}

/*
dist_type merge_kNN_buffer(kNN_buffer *b, kNN_buffer *pool[], int n)
{
	int i, head[n], k = b->k, m, min_t;
	
	for(m = 0; m < n; m++) {
		head[m] = 0;
	}

	for(i = 0; i < k; i++) {
		b->buff[i].dist = INT_MAX;
		min_t = 0;
		for(m = 0; m < n; m++) {
			if(pool[m]->num > head[m] && pool[m]->buff[head[m]].dist < b->buff[i].dist) {
				b->buff[i] = pool[m]->buff[head[m]];
				min_t = m;
			}
		}
		head[min_t]++;
	}
	b->num = k;
	b->k_nearest = b->buff[k - 1].dist;
	return b->k_nearest;
}
*/

answer_type *get_kNN_buffer(kNN_buffer *b, int i)
{
	return &(b->buff[i]);
}

// （現状）最近傍解のみ取り込んでいる．→100NNまでcsvに保存されている
answer_type *read_correct_answer(char *answer_csv_file, int num_queries)
{
	FILE *afp ;
    char buf[100000]={0};
	int i, q;
	answer_type *ans = (answer_type *)malloc(sizeof(answer_type) * num_queries);
	
	afp=fopen(answer_csv_file,"r");
	if(afp == NULL){
		fprintf(stderr, "cannot open answer csv file = %s\n", answer_csv_file);
		exit(0);
	}

	// 1行読み捨てる
	if(fgets(buf, MAX_LEN, afp) == NULL) {
		fprintf(stderr, "read error: answer csv file = %s\n", answer_csv_file);
		exit(0);
	}
	// もう1行読み捨てる
	if(fgets(buf, MAX_LEN, afp) == NULL) {
		fprintf(stderr, "read error: answer csv file = %s\n", answer_csv_file);
		exit(0);
	}
	// answer index, dist を読み込む
    for(i = 0; fgets(buf, MAX_LEN, afp) != NULL && i < num_queries; i++){
		q = atoi(strtok(buf, ","));
    	if(q != i) {
    		fprintf(stderr, "invalid answer file (line number != query number): answer csv file = %s\n", answer_csv_file);
			exit(0);
    	}
    	strtok(NULL, ","); // auxi_type query_id = atol(strtok(NULL, ","));
		ans[i].data_num = atoi(strtok(NULL, ","));
		ans[i].dist = atoi(strtok(NULL, ","));
		//ans[i].data_id = atol(strtok(NULL, ","));
    }
	fclose(afp); 

	if(i != num_queries) { 
		fprintf(stderr, "query_num != answer_num\n");
		exit(0);
	}
    
	return ans;
}

void sort_answer(int num, answer_type ans[])
{
	qsort(ans, num, sizeof(answer_type), comp_answer);
}

void search_kNN(dataset_handle *dh, query_type *qr, int num_candidates, int data_num_of_candidate[], kNN_buffer *top_k) // 解候補から top-K を求める（第２段階検索）
{
	#if defined(_OPENMP) && NUM_THREADS > 1
	omp_set_num_threads(NUM_THREADS);
	int num_k = top_k->k;
	int nt = omp_get_max_threads(); 	// スレッド数を求める
	kNN_buffer *b_pool[nt];
	dist_type k_nearest[nt];
	for(int t = 0; t < nt; t++) {
		if((b_pool[t] = new_kNN_buffer(num_k)) == NULL) {
			fprintf(stderr, "cannot allocate new kNN buffer\n");
			exit(0);
		}
	}
	#else
	dist_type k_nearest;
	#endif

	SET_DIST(qr->ftr); // 距離はすべて質問からになるので，片方を質問に固定
	#if defined(_OPENMP) && NUM_THREADS > 1
	for(int t = 0; t < nt; t++) {
		k_nearest[t] = make_empty_kNN_buffer(b_pool[t]);
		if(dh->ftr_on == SECONDARY_MEMORY) dh->mf[t]->read_in = 0;
	}
	#pragma omp parallel for
	#else
	k_nearest = make_empty_kNN_buffer(top_k);
	if(dh->ftr_on == SECONDARY_MEMORY) dh->mf[0]->read_in = 0;
	#endif
	for(int i = 0; i < num_candidates; i++) {
		struct_multi_ftr *mf = NULL;
		#if defined(_OPENMP) && NUM_THREADS > 1
		int t = omp_get_thread_num();
		kNN_buffer *b = b_pool[t];
		if(dh->ftr_on == SECONDARY_MEMORY) mf = dh->mf[t];
		#else
		kNN_buffer *b = top_k;
		if(dh->ftr_on == SECONDARY_MEMORY) mf = dh->mf[0];
		#endif
		answer_type ans;
		ans.data_num = data_num_of_candidate[i];
//		long data_id;
		if(dh->ftr_on == MAIN_MEMORY) {
			ans.dist = DISTANCE_22(dh->ds->ftr_id[data_num_of_candidate[i]].ftr);
//			data_id = dh->ds->ftr_id[data_num_of_candidate[i]].data_id;
		} else {
			struct_ftr_id *ftr_id_p = get_next_ftr_id_from_multi_ftr(mf, data_num_of_candidate, i, num_candidates);
			ans.dist = DISTANCE_22(ftr_id_p->ftr);
//			data_id = ftr_id_p->data_id;
//			ans.dist = DISTANCE_22(get_next_ftr_id_from_multi_ftr(dh->mf, data_num_of_candidate, i, num_candidates)->ftr);
		}
//		printf("$$ i, %d, num, %d, dist, %d, data_id, %ld\n", i, ans.data_num, ans.dist, data_id);
		#if defined(_OPENMP) && NUM_THREADS > 1
		if(ans.dist < k_nearest[t]) {
			k_nearest[t] = push_kNN_buffer(&ans, b);
		}
		#else
		if(ans.dist < k_nearest) {
			k_nearest = push_kNN_buffer(&ans, b);
		}
		#endif
	}
//	exit(0);
	#if defined(_OPENMP) && NUM_THREADS > 1
	#pragma omp parallel for
	for(int t = 0; t < nt; t++) {
		k_nearest[t] = flush_kNN_buffer(b_pool[t]);
	}
	merge_kNN_buffer(top_k, b_pool, nt);
//	printf("top_dist = %d, k_nearest = %d\n", top_k->buff[0].dist, top_k->k_nearest);
	#else
	k_nearest = final_flush_kNN_buffer(top_k);
//	printf("top_dist = %d, k_nearest = %d\n", top_k->buff[0].dist, top_k->k_nearest);
//	printf("k_nearest = %d\n", k_nearest);
	#endif
}

void search_NN(dataset_handle *dh, query_type *qr, int num_candidates, int data_num_of_candidate[], kNN_buffer *top_k) // 解候補から top-K を求める（第２段階検索）
{
	#if defined(_OPENMP) && NUM_THREADS > 1
	omp_set_num_threads(NUM_THREADS);
	int nt = omp_get_max_threads(); 	// スレッド数を求める
	answer_type ans[nt];
	dist_type k_nearest[nt];
	#else
	answer_type ans;
	dist_type k_nearest;
	#endif

	SET_DIST(qr->ftr); // 距離はすべて質問からになるので，片方を質問に固定
	#if defined(_OPENMP) && NUM_THREADS > 1
	for(int t = 0; t < nt; t++) {
		k_nearest[t] = INT_MAX;
		if(dh->ftr_on != MAIN_MEMORY) {
			dh->mf[t]->read_in = 0;
		}
	}
	#pragma omp parallel for
	#else
	k_nearest = INT_MAX;
	if(dh->ftr_on != MAIN_MEMORY) {
		dh->mf[0]->read_in = 0;
	}
	#endif
	for(int i = 0; i < num_candidates; i++) {
		#if defined(_OPENMP) && NUM_THREADS > 1
		int t = omp_get_thread_num();
		answer_type *a = &ans[t];
		struct_multi_ftr *mf = dh->mf[t];
		#else
		answer_type *a = &ans;
		struct_multi_ftr *mf = dh->mf[0];
		#endif
//		answer_type ans;
//		a->data_num = data_num_of_candidate[i];
		int dist;
		if(dh->ftr_on == MAIN_MEMORY) {
			dist = DISTANCE_22(dh->ds->ftr_id[data_num_of_candidate[i]].ftr);
//			printf("i = %d, dist = %d\n", i, a->dist);
		} else {
			dist = DISTANCE_22(get_next_ftr_id_from_multi_ftr(mf, data_num_of_candidate, i, num_candidates)->ftr);
		}
		#if defined(_OPENMP) && NUM_THREADS > 1
		if(dist < k_nearest[t]) {
			a->data_num = data_num_of_candidate[i];
			a->dist = dist;
			k_nearest[t] = dist;
		}
		#else
		if(dist < k_nearest) {
			a->data_num = data_num_of_candidate[i];
			a->dist = dist;
			k_nearest = dist;
		}
		#endif
	}
	#if defined(_OPENMP) && NUM_THREADS > 1
	// 各スレッドが見つけた暫定解から距離最小のものを選ぶ
	int min_t = 0;
	dist_type min_dist = UINT_MAX;
	for(int t = 0; t < nt; t++) {
		if(ans[t].dist < min_dist) {
			min_dist = ans[t].dist;
			min_t = t;
		}
	}
	top_k->buff[0] = ans[min_t];
	#else
	top_k->buff[0] = ans;
	#endif
	top_k->num = 1;
	top_k->k_nearest = top_k->buff[0].dist;
}

void out_result(char *filename, int num_queries, answer_type ans[], kNN_buffer *top_k[])
{
	int i;
	FILE *fp;
	char name[MAX_LEN] = "";
	char temp[MAX_LEN];
	
	strcat(name, filename);
	strcpy(temp, name);
	char *temp2 = strtok(name, "." );
	i = 1;
	while((fp = fopen(temp, "r")) != NULL){   // resultファイル名を上書きしないようにする
		fclose(fp);
		char str[MAX_LEN] = {0};
		memset(temp, '\0', MAX_LEN);
		strcpy(temp, temp2);
		sprintf(str, "%d" , i);
		strcat(temp, "_");
		strcat(temp, str);
		strcat(temp, ".csv");
		i++;
	}

	fp = fopen(temp, "w");
	if(fp == NULL) {
		printf("ファイルが開けません（filename = %s, temp = %s）\n", filename, temp);
		return;
	}
	fprintf(fp, "nearest_idx, nearest_dist, ans_idx[0], ans_dist[0], ans_idx[1], ans_dist[1], ... \n");
	for(int i = 0; i < num_queries; i++) {
		fprintf(fp, "%d,%d,", ans[i].data_num, ans[i].dist);
		int k;
		for(k = 0; k < top_k[i]->k - 1; k++) {
			fprintf(fp, "%d, %d,", top_k[i]->buff[k].data_num, top_k[i]->buff[k].dist);
		}
		fprintf(fp, "%d, %d\n", top_k[i]->buff[k].data_num, top_k[i]->buff[k].dist);
	}
	fclose(fp);
}

void out_result2(char *filename, int w, int d, long sec, long nsec, int num_c, int num_queries, answer_type ans[], kNN_buffer *top_k[], int sorted)
{
	int i;
	FILE *fp;
	char name[MAX_LEN] = "";
	char temp[MAX_LEN];
	double sum = 0;
	strcat(name, filename);
	strcpy(temp, name);
	char *temp2 = strtok(name, "." );
	i = 1;
	while((fp = fopen(temp, "r")) != NULL){   // resultファイル名を上書きしないようにする
		fclose(fp);
		char str[MAX_LEN] = {0};
		memset(temp, '\0', MAX_LEN);
		strcpy(temp, temp2);
		sprintf(str, "%d" , i);
		strcat(temp, "_");
		strcat(temp, str);
		strcat(temp, ".csv");
		i++;
	}

	fp = fopen(temp, "w");
	if(fp == NULL) {
		printf("ファイルが開けません（filename = %s, temp = %s）\n", filename, temp);
		return;
	}

	for(int i = 0; i < num_queries; i++) {
		if(ans[i].dist == top_k[i]->buff[0].dist) {
			sum++;
		}
	}

	#ifdef NUM_THREADS
	int nt = NUM_THREADS;
	#else
	int nt = 1;
	#endif
	
	#ifdef SEQUENTIAL_FILTERING
	char *m = "SF";
	#endif
	#ifdef SEQUENTIAL_FILTERING_USING_BUCKET
	char *m = "BKT";
	#endif
	#ifdef SEQUENTIAL_FILTERING_USING_HAMMING
	char *m = "HAMMING";
	#endif
	#ifdef FILTERING_BY_SKETCH_ENUMERATION_C2N
	char *m = "ENU";
	#endif
	#ifdef WITHOUT_FILTERING
	char *m = "NO";
	#endif
	#ifdef DOUBLE_FILTERING
	char *m = "DF";
	#endif
	

	fprintf(fp, "w, dim, nt, time, k'/n(ppm), recall, method, sort\n");
	fprintf(fp, "%d, %d, %d, %ld.%09ld, %d, %f, %s, %d, mark\n", w, d, nt, sec, nsec, num_c, sum / num_queries * 100, m, sorted);

	fclose(fp);
}

double out_result_double(char *filename, int n_w, int e_w, int d, double etime, int nc_1st, int nc_2nd, int num_queries, answer_type ans[], kNN_buffer *top_k[])
{
	int i;
	FILE *fp;
	char name[MAX_LEN] = "";
	char temp[MAX_LEN];
	double sum = 0;
	strcat(name, filename);
	strcpy(temp, name);
	char *temp2 = strtok(name, "." );
	i = 1;
	while((fp = fopen(temp, "r")) != NULL){   // resultファイル名を上書きしないようにする
		fclose(fp);
		char str[MAX_LEN] = {0};
		memset(temp, '\0', MAX_LEN);
		strcpy(temp, temp2);
		sprintf(str, "%d" , i);
		strcat(temp, "_");
		strcat(temp, str);
		strcat(temp, ".csv");
		i++;
	}

	fp = fopen(temp, "w");
	if(fp == NULL) {
		printf("ファイルが開けません（filename = %s, temp = %s）\n", filename, temp);
		return 0;
	}

	for(int i = 0; i < num_queries; i++) {
		if(ans[i].dist == top_k[i]->buff[0].dist) {
			sum++;
		}
	}

	#ifdef NUM_THREADS
	int nt = NUM_THREADS;
	#else
	int nt = 1;
	#endif
	
	#ifdef SEQUENTIAL_FILTERING
	char *m = "SF";
	#endif
	#ifdef SEQUENTIAL_FILTERING_USING_BUCKET
	char *m = "BKT";
	#endif
	#ifdef SEQUENTIAL_FILTERING_USING_HAMMING
	char *m = "HAMMING";
	#endif
	#ifdef FILTERING_BY_SKETCH_ENUMERATION_C2N
	char *m = "ENU";
	#endif
	#ifdef WITHOUT_FILTERING
	char *m = "NO";
	#endif
	#ifdef DOUBLE_FILTERING
	char *m = "DF";
	#endif
	
	#ifdef SCORE_P_1ST
	fprintf(fp, "n_w, e_w, dim, nt, time, p_1,  k_1'/n(ppm), p_2, k_2'/n(ppm), recall, method, query\n");
	fprintf(fp, "%d, %d, %d, %d, %.9lf, %lf, %d, %lf, %d, %lf, %s, %s, mark\n", n_w, e_w, d, nt, etime, SCORE_P_1ST / 10.0, nc_1st, SCORE_P_2ND / 10.0, nc_2nd, sum / num_queries * 100, m, QUERY_FILE);
	printf("n_w, e_w, dim, nt, time, p_1,  k_1'/n(ppm), p_2, k_2'/n(ppm), recall, method, query\n");
	printf("%d, %d, %d, %d, %.9lf, %lf, %d, %lf, %d, %lf, %s, %s, mark\n", n_w, e_w, d, nt, etime, SCORE_P_1ST / 10.0, nc_1st, SCORE_P_2ND / 10.0, nc_2nd, sum / num_queries * 100, m, QUERY_FILE);
	#endif

	fclose(fp);

	return sum / num_queries * 100;
}
