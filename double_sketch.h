#pragma once
// スケッチの型定義と基本操作およびスケッチを用いるkNN検索に関するもののみ
// 距離関数は特徴データに関するものをまとめた ftr に置く
// ピボット選択（QBPやAIR, LS）に関するものは pivot_selection にまとめる

// 前提
// 定数マクロ
// FTR_DIM = 特徴データの次元
// PJT_DIM = 射影次元．ここではピボットの幅（ビット数）
// PIVOT_PARTITION = GHP | BP | QBP | PQBP （基礎分割関数）（とりあえずQBPのみなので無視）

#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "ftr.h"
#include "kNN_search.h"


#ifndef NARROW_PJT_DIM
#define NARROW_PJT_DIM		24				// width of narrow sketches for 1st filtering
#endif
#ifndef EXPANDED_PJT_DIM
#define EXPANDED_PJT_DIM	512				// width of expanded sketches for 2nd filtering
#endif

#define NARROW_SKETCH_SIZE	1				// スケッチの大きさ（32ビット単位）
#define NARROW_TABLE_SIZE	4				// score計算のための表関数の表の数（8ビット毎に256の大きさの表）
#define EXPANDED_SKETCH_SIZE ((EXPANDED_PJT_DIM + 63) / 64)  // スケッチの大きさ（64ビット単位）
#ifdef EXPANDED_TABLE_WIDE // 表関数を16ビット対応にするときは，EXPANDED_TABLE_WIDEをdefineする
#define EXPANDED_TABLE_BIT 16
#else
#define EXPANDED_TABLE_BIT 8
#endif
#define EXPANDED_TABLE_SIZE (EXPANDED_SKETCH_SIZE * 64 / EXPANDED_TABLE_BIT)

typedef unsigned int narrow_sketch_type;	// スケッチの型（32ビットまで）
typedef unsigned long expanded_sketch_type[EXPANDED_SKETCH_SIZE];	// スケッチの型（65ビット以上）

#define PJT_DIM EXPANDED_PJT_DIM
#ifndef NUM_PART
#define NUM_PART PJT_DIM
#endif
#define PART_PJT_MIN (PJT_DIM / NUM_PART)
#define PART_PJT_MAX ((PJT_DIM + NUM_PART - 1) / NUM_PART)

// PART_NUM(p) = 射影次元（p = 0, ... , PJT_DIM - 1）に対応する部分空間番号
#define PART_NUM(p) (((p) / PART_PJT_MAX) < (PJT_DIM % NUM_PART) ? ((p) / PART_PJT_MAX) : ((p) - PJT_DIM % NUM_PART) / PART_PJT_MIN)
// PART_START(p), PART_DIM(p), PART_END(p), PART_PJT_DIM(p) 射影次元（p = 0, ... , PJT_DIM - 1）に対応する部分空間の
// 開始次元番号, 次元数, 最終次元番号, 射影次元数
#define PART_START(j) ((FTR_DIM / NUM_PART) * PART_NUM(j) + (PART_NUM(j) < FTR_DIM % NUM_PART ? PART_NUM(j) : FTR_DIM % NUM_PART))
#define PART_DIM(j) ((FTR_DIM / NUM_PART) + (PART_NUM(j) < FTR_DIM % NUM_PART ? 1 : 0))
#define PART_END(j) (PART_START(j) + PART_DIM(j) - 1)
#define PART_PJT_DIM(j) ((PJT_DIM / NUM_PART) + (PART_NUM(j) < PJT_DIM % NUM_PART ? 1 : 0))

/*
p_dim   part_num        part_start      part_end        part_dim        part_pjt_dim
0 - 3   0               0               10              11              4
4 - 7   1               11              21              11              4
8 - 10  2               22              32              11              3
11 - 13 3               33              43              11              3
14 - 16 4               44              53              10              3
17 - 19 5               54              63              10              3
*/

typedef struct {
	ftr_type  p[NARROW_PJT_DIM];				// BP, QBP の中心点
	dist_type r[NARROW_PJT_DIM];				// BP, QBP の半径
	int type;									// 基礎分割関数（GHP = 0, BP = 1, QBP = 2, PQBP = 3）
} narrow_pivot_type;

typedef struct {
	ftr_type  p[EXPANDED_PJT_DIM];				// BP, QBP の中心点
	dist_type r[EXPANDED_PJT_DIM];				// BP, QBP の半径
	int type;									// 基礎分割関数（GHP = 0, BP = 1, QBP = 2, PQBP = 3）
} expanded_pivot_type;

typedef struct {
	query_type query;
	narrow_sketch_type sketch;					// 質問点のスケッチ
	int bd[NARROW_PJT_DIM];						// 質問点と分割境界との距離（距離下限やscoreの計算に使用）
	int idx[NARROW_PJT_DIM];					// bd の順位表
	dist_type tbl[NARROW_TABLE_SIZE][256]; 		// scoreの計算に用いる表関数
} struct_query_narrow_sketch;

typedef struct {
	query_type query;
	expanded_sketch_type sketch;				// 質問点のスケッチ
	int bd[EXPANDED_PJT_DIM];					// 質問点と分割境界との距離（距離下限やscoreの計算に使用）
	int idx[EXPANDED_PJT_DIM];					// bd の順位表
	dist_type tbl[EXPANDED_TABLE_SIZE][1 << EXPANDED_TABLE_BIT]; 	// scoreの計算に用いる表関数
} struct_query_expanded_sketch;

typedef struct {
	answer_type *answer;
	narrow_sketch_type narrow_sketch;
	expanded_sketch_type expanded_sketch;
} struct_answer_sketch;

typedef struct {
	narrow_sketch_type sk;		// sketch
	int num;			// number of data whose sketch is s
	int pos;			// offset position in sftr
} narrow_sk_num_pair;

typedef struct {
	expanded_sketch_type sk;		// sketch
	int num;			// number of data whose sketch is s
	int pos;			// offset position in sftr
} expanded_sk_num_pair;

typedef struct {
	int num_data; 				// the number of data points in dataset
	ftr_type *ftr_data;			// array of ftr data (NULL, if not used)
	narrow_sketch_type *sk;			// array of sketches (NULL, if not used)
	int *idx; 					// idx[i] = the original position of the i-th data in sketch order (NULL, if not used)
	int *bkt;					// bkt[s] = The first position of the data whose sketch is s in the data arranged in the sketch order (NULL, if not used)
	int num_nonempty_buckets;	// number of nonempty buckets = number of sk_num_pairs
	narrow_sk_num_pair *sk_num;		// representing nonempty buckets (NULL, if not used)
} struct_narrow_bucket;

typedef struct {
	int num_data; 				// the number of data points in dataset
	ftr_type *ftr_data;			// array of ftr data (NULL, if not used)
	expanded_sketch_type *sk;			// array of sketches (NULL, if not used)
	int *idx; 					// idx[i] = the original position of the i-th data in sketch order (NULL, if not used)
//	int *bkt;					// bkt[s] = The first position of the data whose sketch is s in the data arranged in the sketch order (NULL, if not used)
	int num_nonempty_buckets;	// number of nonempty buckets = number of sk_num_pairs
	expanded_sk_num_pair *sk_num;		// representing nonempty buckets (NULL, if not used)
} struct_expanded_bucket;

#define QSIZE  (1 << NARROW_PJT_DIM)

typedef struct {
    dist_type key;
	narrow_sketch_type sk;
	unsigned idx;
} QUE;

typedef struct {
	QUE element[QSIZE + 1];
	int qsize;
} struct_que;

// カーソル使用（imamura）
typedef struct {
    dist_type key;
    unsigned cursor;
} QUE_c2;

// カーソル使用（imamura）mnodified by Takeshi (based on Naoya's idea)
typedef struct {
	narrow_sketch_type sk; // sketch (or sketch ^ query sketch)
	// ここには，スケッチ（または，スケッチと質問スケッチのXOR）
	narrow_sketch_type pt; // sketch ^ query sketch in rearranged bit order
	// ここには，スケッチと質問スケッチのXORをビットを距離下限順に並べ替えたもの
	// 最初から見ると，0, 1, 10, 11, 100, ... のようになるはず
} QUE_Detail_n;

typedef struct {
    int qsize;
    int detail_size;
    QUE_c2 element[QSIZE + 1];
    QUE_Detail_n details[QSIZE + 1];
} struct_que_c2_n;

narrow_sketch_type 		data_to_narrow_sketch(ftr_type o, narrow_pivot_type *pivot);
void 					data_to_narrow_sketch_1bit(ftr_type o, narrow_pivot_type *pivot, int dim, narrow_sketch_type *sk);

void 					data_to_expanded_sketch(ftr_type o, expanded_pivot_type *pivot, expanded_sketch_type sk);
void 					data_to_expanded_sketch_1bit(ftr_type o, expanded_pivot_type *pivot, int dim, expanded_sketch_type sk);

narrow_pivot_type 		*new_narrow_pivot(int type);
void 					read_narrow_pivot(char *filename, narrow_pivot_type *pivot);
void 					write_narrow_pivot(char *filename, narrow_pivot_type *pivot);

struct_narrow_bucket 	*new_narrow_bucket(int num_data, narrow_sketch_type sketch[]);
struct_narrow_bucket 	*read_narrow_bucket(char *filename);
struct_narrow_bucket 	*read_compact_narrow_bucket(char *filename);
void 					write_narrow_bucket(char *filename, struct_narrow_bucket *b);
void 					free_narrow_bucket(struct_narrow_bucket *b);

expanded_pivot_type 	*new_expanded_pivot(int type);
void 					read_expanded_pivot(char *filename, expanded_pivot_type *pivot);
void 					write_expanded_pivot(char *filename, expanded_pivot_type *pivot);

struct_expanded_bucket 	*new_expanded_bucket(int num_data, expanded_sketch_type sketch[]);
struct_expanded_bucket 	*read_expanded_bucket(char *filename);
struct_expanded_bucket 	*read_compact_expanded_bucket(char *filename);
void 					write_expanded_bucket(char *filename, struct_expanded_bucket *b);
void 					free_expanded_bucket(struct_expanded_bucket *b);

int 					find_pivot_for_expanded_sketch(int idx[], expanded_sketch_type sk[], int i, int j);
int 					partition_by_pivot_for_expanded_sketch(int idx[], expanded_sketch_type sk[], int i, int j, expanded_sketch_type piv);
void 					quick_sort_for_expanded_sketch(int idx[], expanded_sketch_type sk[], int i, int j);

void 					sort_expanded_sketches_narrow_sketch_order(expanded_sketch_type esk[], int idx[], int num);

void min_heapify_p(int i, struct_que *que);
int deq_p(QUE *q, struct_que *que);
void enq_p(QUE *q, struct_que *que);

void make_empty_que_c2_n(struct_que_c2_n *que);
int new_que_e2_n(struct_que_c2_n *que);
void min_heapify_c2_n(int i, struct_que_c2_n *que);
int deq_c2_n(QUE_c2 *qe, struct_que_c2_n *que);
void deq_c2_n_del(struct_que_c2_n *que);
void enq_c2_n(QUE_c2 *qe, struct_que_c2_n *que);
void enq_c2_n_after_deq(QUE_c2 *qe, struct_que_c2_n *que);

// struct_query_narrow_sketch *make_query_narrow_sketch(struct_dataset *ds_query, narrow_pivot_type *pivot);
// void set_query_narrow_sketch(struct_query_narrow_sketch *qs, query_type *query, narrow_pivot_type *pivot);
void set_query_narrow_sketch_p(struct_query_narrow_sketch *qs, query_type *query, narrow_pivot_type *pivot, double p);
dist_type narrow_priority(narrow_sketch_type s, struct_query_narrow_sketch *qs);

// dist_type narrow_hamming(narrow_sketch_type s, narrow_sketch_type t);
// void filtering_by_sequential_search_using_kNN_buffer(struct_query_sketch *qs, int num_data, narrow_sketch_type sketch[], kNN_buffer *buff, int data_num[], int num_candidates);
// void filtering_by_sequential_search_using_quick_select_k(struct_query_sketch *qs, int num_data, sketch_type sk[], dist_type sc[], int data_num[], int num_candidates);

// #if defined(NARROW_SKETCH)
void filtering_by_sketch_enumeration(struct_query_narrow_sketch *qs, struct_narrow_bucket *bucket, struct_que *que, int data_num[], int num_candidates);
void filtering_by_sketch_enumeration_c2_n(struct_query_narrow_sketch *qs, struct_narrow_bucket *bucket, struct_que_c2_n *que, int data_num[], int num_candidates);

// struct_query_expanded_sketch *make_query_expanded_sketch(struct_dataset *ds_query, expanded_pivot_type *pivot);
// void set_query_expanded_sketch(struct_query_expanded_sketch *qs, query_type *query, expanded_pivot_type *pivot);
void set_query_expanded_sketch_p(struct_query_expanded_sketch *qs, query_type *query, expanded_pivot_type *pivot, double p);
dist_type expanded_priority(expanded_sketch_type s, struct_query_expanded_sketch *qs);


// #endif
//void filtering_by_sequential_search_using_compact_bucket(struct_query_sketch *qs, int num_data, compact_bucket *bucket_ds, dist_type score[], int idx[], int data_num[], int num_candidates);
//void filtering_by_sequential_search_n(int num_queries, struct_query_sketch qs[], int num_data, sketch_type sketch[], kNN_buffer *can[]);
//void filtering_by_sequential_search_using_bucket();
//void filtering_by_sketch_enumeration();


