#pragma once
// basic definitions for k-NN search (Metric space, query_type and answer_type, kNN_buffer to select top-k)
#include <limits.h>
#include "ftr.h"

// Metric space
typedef enum {EUCLID, L_1} metric;
// when L_P is used, ativate the following, add necessary program code, and compile with option like "-DL_P=1.3".
//#ifndef L_P
//#define L_P 1.3
//#endif

#ifndef MERTIC
#define METRIC EUCLID // DeCAF descriptor assumes Euclidean
#endif

typedef unsigned int dist_type;

#if METRIC == EUCLID
#define DISTANCE    dist_L2
#define DISTANCE_2  dist_L2_2
#define DISTANCE_22 dist_L2_22
#define SET_DIST    set_dist_L2_22
#ifdef PARTITION_TYPE_PQBP
#define PART_DISTANCE    part_dist_L2
#define PART_DISTANCE_2  part_dist_L2_2
#define PART_DISTANCE_22 part_dist_L2_22
#define SET_PART_DIST    set_part_dist_L2_22
#endif
#else
#define DISTANCE    dist_L1
#define DISTANCE_2  dist_L1_2
#define DISTANCE_22 dist_L1_22
#define SET_DIST    set_dist_L2_22 // set function is common to L2
#ifdef PARTITION_TYPE_PQBP
#define PART_DISTANCE    part_dist_L1
#define PART_DISTANCE_2  part_dist_L1_2
#define PART_DISTANCE_22 part_dist_L1_22
#define SET_PART_DIST    set_part_dist_L2_22 // set function is common to L2
#endif
#endif

typedef struct {
	int data_num;			// データ番号（ftrファイル内での通し番号）
	dist_type dist;			// 質問との距離（フィルタリングでは，score_p）
	// auxi_type data_id;		// データID（ftrの補助情報）（処理が重たくなるので削除）
} answer_type;

typedef struct {
	int query_num;			// 質問番号（質問のftrファイル内での通し番号）
	ftr_type ftr;			// 特徴データ
	// auxi_type query_id;		// 質問ID（ftrの補助情報）（処理が重たくなるので削除）
} query_type;

typedef struct {
	answer_type *buff;		// 解 (id, dist) を入れるバッファー（大きさは 2k）
	int k, num;				// バッファーの大きさ k と，格納している解の個数
	dist_type k_nearest;	// k-NN の距離（ただし，最後にソートして
} kNN_buffer;

dist_type dist_L1(ftr_type a, ftr_type b, int dim);
dist_type dist_L1_2(ftr_type a, ftr_type b, int dim, dist_type dist);
void set_dist_L1_22(ftr_type a);
// dist_type dist_L1_22(ftr_type b); // set function is common to L2

dist_type dist_L2(ftr_type a, ftr_type b, int dim);
dist_type dist_L2_2(ftr_type a, ftr_type b, int dim, dist_type dist);
void set_dist_L2_22(ftr_type a);
dist_type dist_L2_22(ftr_type b);

#ifdef PARTITION_TYPE_PQBP
dist_type part_dist_L2(ftr_type a, ftr_type b, int dim_start, int dim);
dist_type part_dist_L2_2(ftr_type a, ftr_type b, int dim_start, int dim, dist_type dist);
void set_part_dist_L2_22(ftr_type a, int dim_start, int dim);
dist_type part_dist_L2_22(ftr_type b, int dim_start, int dim);

#endif

int find_pivot_answer(answer_type ans[], int i, int j);
int partition_by_pivot_answer(answer_type ans[], int i, int j, dist_type piv);
void quick_sort_answer(answer_type ans[], int i, int j);
void quick_select_k_r_answer(answer_type ans[], int i, int j, int k);
void quick_select_k_answer(answer_type ans[], int i, int j, int k);

kNN_buffer *new_kNN_buffer(int k);
void free_kNN_buffer(kNN_buffer *b);
dist_type make_empty_kNN_buffer(kNN_buffer *b);
dist_type push_kNN_buffer(answer_type *a, kNN_buffer *b);
int comp_answer(const void *a, const void *b);
dist_type flush_kNN_buffer(kNN_buffer *b);
dist_type final_flush_kNN_buffer(kNN_buffer *b);
dist_type merge_kNN_buffer(kNN_buffer *b, kNN_buffer *pool[], int n);
answer_type *get_kNN_buffer(kNN_buffer *b, int i);
answer_type *read_correct_answer(char *answer_csv_file, int num_queries);
void sort_answer(int num, answer_type ans[]);
void search_kNN(dataset_handle *dh, query_type *qr, int num_c, int candidate[], kNN_buffer *top_k);
void search_NN(dataset_handle *dh, query_type *qr, int num_c, int candidate[], kNN_buffer *top_k);

void out_result(char *filename, int num_queries, answer_type ans[], kNN_buffer *top_k[]);
void out_result2(char *filename, int w, int d, long, long, int num_c, int num_queries, answer_type ans[], kNN_buffer *top_k[], int sorted);
double out_result_double(char *filename, int n_w, int e_w, int d, double, int nc_1st, int nc_2nd, int num_queries, answer_type ans[], kNN_buffer *top_k[]);

