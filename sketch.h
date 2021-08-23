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

/*
#ifndef WIDE_SKETCH
#ifndef NARROW_SKETCH
#define NARROW_SKETCH
#endif
#endif
*/

#if defined(NARROW_SKETCH)
	#define SKETCH_SIZE 1 // スケッチの大きさ（32ビット単位）
	#define TABLE_SIZE 4  // score計算のための表関数の表の数（8ビット毎に256の大きさの表）
	typedef unsigned int sketch_type;	// スケッチの型（32ビットまで）
#elif defined(WIDE_SKETCH)
	#define SKETCH_SIZE 1 // スケッチの大きさ（64ビット単位）
	#define TABLE_SIZE 8  // score計算のための表関数の表の数（8ビット毎に256の大きさの表）
	typedef unsigned long sketch_type;	// スケッチの型（64ビットまで）
#elif defined(EXPANDED_SKETCH)
	#define SKETCH_SIZE ((PJT_DIM + 63) / 64)  // スケッチの大きさ（64ビット単位）
	#define TABLE_SIZE (SKETCH_SIZE * 8)
	typedef unsigned long sketch_type[SKETCH_SIZE];
#endif

/*
#ifdef PARTITION_TYPE_PQBP // 座標分割QBP（PQBP）のときは，FTR_DIMをPJT_DIM個に分割する．
// FTR_DIMがPJT_DIMで割り切れないときは，最初の部分空間に1個ずつ座標を加える．
// （例）FTR_DIM = 64, PJT_DIM = 6 のときは，10次元ずつで4次元分の余りがでる．
//  最初の4空間（0から3）までは，11次元，残りは10次元の部分空間とする．
//  0: 0 - 10, 1: 11 - 21, 2: 22 - 32, 3: 33 - 43, 4: 44 - 53, 6: 54 - 63  
#define PART_START(j) ((FTR_DIM / PJT_DIM) * j + (j < FTR_DIM % PJT_DIM ? j : 0)) // 部分座標空間の開始次元
#define PART_DIM(j) ((FTR_DIM / PJT_DIM) + (j < FTR_DIM % PJT_DIM ? 1 : 0)) // 部分座標空間の次元数
#endif
*/

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
	ftr_type  p[PJT_DIM];	// BP, QBP の中心点
	dist_type r[PJT_DIM];	// BP, QBP の半径
	int type;				// 基礎分割関数（GHP = 0, BP = 1, QBP = 2, PQBP = 3）（現状ではQBPのみ）
} pivot_type;

typedef struct {
	query_type query;
	sketch_type sketch;		// 質問点のスケッチ
	int bd[PJT_DIM];		// 質問点と分割境界との距離（距離下限やscoreの計算に使用）
	int idx[PJT_DIM];		// bd の順位表
	dist_type tbl[TABLE_SIZE][256]; 	// scoreの計算に用いる表関数
} struct_query_sketch;

typedef struct {
	answer_type *answer;
	sketch_type sketch;
} struct_answer_sketch;

typedef struct {
	sketch_type sk;		// sketch
	int num;			// number of data whose sketch is s
	int pos;			// offset position in sftr
} sk_num_pair;

typedef struct {
	int num_data; 				// the number of data points in dataset
	ftr_type *ftr_data;			// array of ftr data (NULL, if not used)
	sketch_type *sk;			// array of sketches (NULL, if not used)
	int *idx; 					// idx[i] = the original position of the i-th data in sketch order (NULL, if not used)
	#ifdef NARROW_SKETCH
	int *bkt;					// bkt[s] = The first position of the data whose sketch is s in the data arranged in the sketch order (NULL, if not used)
	#endif
	int num_nonempty_buckets;	// number of nonempty buckets = number of sk_num_pairs
	sk_num_pair *sk_num;		// representing nonempty buckets (NULL, if not used)
} struct_bucket;

typedef struct {
	char *filename;				// filename of bucket file
	FILE *fp;					// bucket file handler
	int num_data; 				// the number of data points in dataset
	int num_nonempty_buckets;	// number of nonempty buckets = number of sk_num_pairs
	int processed_buckets;		// number of processed buckets (sk_num_pairs)
	sk_num_pair sk_num;			// next pair of sketch and number of records
} struct_bucket_sk_num;

//#ifdef NARROW_SKETCH // Priority queue for sketch enumeration only for NARROW sketches

#if defined(NARROW_SKETCH)

//#define QSIZE  BIT 
#define QSIZE  (1L << PJT_DIM) 

typedef struct {
    dist_type key;
	sketch_type sk;
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
	sketch_type sk; // sketch (or sketch ^ query sketch)
	// ここには，スケッチ（または，スケッチと質問スケッチのXOR）
	sketch_type pt; // sketch ^ query sketch in rearranged bit order
	// ここには，スケッチと質問スケッチのXORをビットを距離下限順に並べ替えたもの
	// 最初から見ると，0, 1, 10, 11, 100, ... のようになるはず
} QUE_Detail_n;

typedef struct {
    int qsize;
    int detail_size;
    QUE_c2 element[QSIZE + 1];
    QUE_Detail_n details[QSIZE + 1];
} struct_que_c2_n;
#endif

#ifdef EXPANDED_SKETCH
void data_to_sketch(ftr_type o, pivot_type *pivot, sketch_type sk);
void data_to_sketch_1bit(ftr_type o, pivot_type *pivot, int dim, sketch_type sk);
#else
sketch_type data_to_sketch(ftr_type o, pivot_type *pivot);
void data_to_sketch_1bit(ftr_type o, pivot_type *pivot, int dim, sketch_type *sk);
#endif
pivot_type *new_pivot(int type);
void read_pivot(char *filename, pivot_type *pivot);
void write_pivot(char *filename, pivot_type *pivot);

struct_bucket *new_bucket(int num_data, sketch_type sketch[]);
void write_bucket(char *filename, struct_bucket *b);
void free_bucket(struct_bucket *b);
struct_bucket *read_bucket(char *filename);
struct_bucket *read_compact_bucket(char *filename);

struct_bucket_sk_num *open_bucket_sk_num(char *filename);
int read_next_bucket_sk_num(struct_bucket_sk_num *bsk);

#if defined(NARROW_SKETCH)
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
#endif

struct_query_sketch *make_query_sketch(struct_dataset *ds_query, pivot_type *pivot);
void set_query_sketch(struct_query_sketch *qs, query_type *query, pivot_type *pivot);
void set_query_sketch_p(struct_query_sketch *qs, query_type *query, pivot_type *pivot, double p);
dist_type priority(sketch_type s, struct_query_sketch *qs);
dist_type priority_partitioned(sketch_type s, struct_query_sketch *qs);
dist_type hamming(sketch_type s, sketch_type t);
void filtering_by_sequential_search_using_kNN_buffer(struct_query_sketch *qs, int num_data, sketch_type sketch[], kNN_buffer *buff, int data_num[], int num_candidates);
void filtering_by_sequential_search_using_quick_select_k(struct_query_sketch *qs, int num_data, sketch_type sk[], dist_type sc[], int data_num[], int num_candidates);

#if defined(NARROW_SKETCH)
void filtering_by_sketch_enumeration(struct_query_sketch *qs, struct_bucket *bucket, struct_que *que, int data_num[], int num_candidates);
void filtering_by_sketch_enumeration_c2_n(struct_query_sketch *qs, struct_bucket *bucket, struct_que_c2_n *que, int data_num[], int num_candidates);
#endif
//void filtering_by_sequential_search_using_compact_bucket(struct_query_sketch *qs, int num_data, compact_bucket *bucket_ds, dist_type score[], int idx[], int data_num[], int num_candidates);
//void filtering_by_sequential_search_n(int num_queries, struct_query_sketch qs[], int num_data, sketch_type sketch[], kNN_buffer *can[]);
//void filtering_by_sequential_search_using_bucket();
//void filtering_by_sketch_enumeration();

int comp_sketch(sketch_type a, sketch_type b);
int find_pivot_for_sketch(int idx[], sketch_type sk[], int i, int j);
int partition_by_pivot_for_sketch(int idx[], sketch_type sk[], int i, int j, sketch_type piv);
void quick_sort_for_sketch(int idx[], sketch_type sk[], int i, int j);


