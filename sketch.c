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
#include <string.h>
#ifdef _OPENMP
#include <omp.h>
#endif
#include "bit_op.h"
#include "sketch.h"
#include "quick.h"

#if defined(NARROW_SKETCH)
#define WRITE_BIT write_bit
#elif defined(WIDE_SKETCH)
#define WRITE_BIT write_bit_long
#else
#define WRITE_BIT write_bit_expanded
#endif


#ifdef EXPANDED_SKETCH

void data_to_sketch(ftr_type o, pivot_type *pivot, sketch_type sk)
{
	for(int i = 0; i < SKETCH_SIZE; i++) sk[i] = 0;
	for(int j = 0; j < PJT_DIM; j++) {
		#ifndef PARTITION_TYPE_PQBP
		WRITE_BIT(j, DISTANCE_2(o, pivot->p[j], FTR_DIM, pivot->r[j]) < pivot->r[j], sk);
		#else
		WRITE_BIT(j, PART_DISTANCE_2(o, pivot->p[j], PART_START(j), PART_DIM(j), pivot->r[j]) < pivot->r[j], sk);
		#endif
	}
}

void data_to_sketch_1bit(ftr_type o, pivot_type *pivot, int dim, sketch_type sp)
{
	#ifndef PARTITION_TYPE_PQBP
	WRITE_BIT(dim, DISTANCE_2(o, pivot->p[dim], FTR_DIM, pivot->r[dim]) < pivot->r[dim], sp);
	#else
	WRITE_BIT(dim, PART_DISTANCE_2(o, pivot->p[dim], PART_START(dim), PART_DIM(dim), pivot->r[dim]) < pivot->r[dim], sp);
	#endif
}

#else // EXPANDED_SKETCH

sketch_type data_to_sketch(ftr_type o, pivot_type *pivot)
{
	sketch_type sk = 0;
	for(int j = 0; j < PJT_DIM; j++) {
		#ifndef PARTITION_TYPE_PQBP
		WRITE_BIT(j, DISTANCE_2(o, pivot->p[j], FTR_DIM, pivot->r[j]) < pivot->r[j], &sk);
		#else
		WRITE_BIT(j, PART_DISTANCE_2(o, pivot->p[j], PART_START(j), PART_DIM(j), pivot->r[j]) < pivot->r[j], &sk);
		#endif
	}
	return sk;
}

void data_to_sketch_1bit(ftr_type o, pivot_type *pivot, int dim, sketch_type *sp)
{
	#ifndef PARTITION_TYPE_PQBP
	WRITE_BIT(dim, DISTANCE_2(o, pivot->p[dim], FTR_DIM, pivot->r[dim])  < pivot->r[dim], sp);
	#else
	WRITE_BIT(dim, PART_DISTANCE_2(o, pivot->p[dim], PART_START(dim), PART_DIM(dim), pivot->r[dim])  < pivot->r[dim], sp);
	#endif
}
#endif // NARROW and WIDE_SKETCH

pivot_type *new_pivot(int type)
{
	pivot_type *pivot = (pivot_type *)malloc(sizeof(pivot_type));
	for(int j = 0; j < PJT_DIM; j++) {
		pivot->p[j] = (ftr_type)malloc(sizeof(ftr_element_type) * FTR_DIM);
		pivot->r[j] = 0;
	}
	pivot->type = type;
	return pivot;
}

void read_pivot(char *filename, pivot_type *pivot)
{
	FILE *pfp ;
    char buf[100000] = {0};
	int i, j;

	pfp=fopen(filename, "r");
	if(pfp == NULL){
		fprintf(stderr, "cannot open pivot file = %s\n", filename);
		exit(0);
	}

	// 基礎分割関数
	if(fgets(buf, MAX_LEN, pfp) != NULL)
		pivot->type = atoi(strtok(buf, ","));

	// 半径
	if(fgets(buf, MAX_LEN, pfp) != NULL) {
		pivot->r[0] = atoi(strtok(buf, ","));
		for(i = 1; i < PJT_DIM; i++)
			pivot->r[i] = atoi(strtok(NULL, ","));
	}
	
	// 中心点
    for(i = 0; fgets(buf, MAX_LEN, pfp) != NULL; i++){
		pivot->p[i][0] = atoi(strtok(buf, ","));
    	for(j = 1; j < FTR_DIM; j++) {
    		pivot->p[i][j] = atoi(strtok(NULL, ","));
    	}
    }
	fclose(pfp); 
}

void write_pivot(char *filename, pivot_type *pivot)
{
	int i, j;
	FILE *fp = fopen(filename, "w");
	if(fp == NULL) {
		printf("cannot write open file = %s\n", filename);
	} else {
		if(pivot->type == PQBP) {
			fprintf(fp, "%d, %d\n", pivot->type, NUM_PART);		// 基礎分割関数, 分割数
		} else {
			fprintf(fp, "%d\n", pivot->type);		// 基礎分割関数
		}
		for(i = 0; i < PJT_DIM - 1; i++)	 	// 半径
			fprintf(fp, "%d,", pivot->r[i]);
		fprintf(fp, "%d\n", pivot->r[i]);
		for(i = 0; i < PJT_DIM; i++) {			// 中心点
			for(j = 0; j < FTR_DIM - 1; j++)
				fprintf(fp, "%d,", pivot->p[i][j]);
			fprintf(fp, "%d\n", pivot->p[i][j]);
		}
	}
	fclose(fp);
}

// 全データのスケッチ配列からバケット表（idx と bkt）を含む構造体（struct_bucket）を作成する
// ただし，PJT_DIM（スケッチ幅）< 32 でのみ使用すること
#ifdef NARROW_SKETCH
struct_bucket *new_bucket(int num_data, sketch_type sk[])
// version 2.0 (using arrays only idx and bucket)
{
	struct_bucket *b = (struct_bucket *)malloc(sizeof(struct_bucket));
	b->num_data = num_data;
	b->ftr_data = NULL;
	b->sk = NULL;

	b->idx = (int *)malloc(sizeof(int) * num_data);
	b->bkt = (int *)calloc((1 << PJT_DIM) + 2, sizeof(int));

	int *bucket = b->bkt + 1;
	for(int i = 0; i < num_data; i++) bucket[sk[i] + 1]++;
	for(int i = 0; i < (1 << PJT_DIM); i++) bucket[i + 1] += bucket[i];
	for(int i = 0; i < num_data; i++) b->idx[bucket[sk[i]]++] = i;

	b->num_nonempty_buckets = 0;
	for(sketch_type s = 0; s < (1 << PJT_DIM); s++) {
		b->num_nonempty_buckets += (b->bkt[s + 1] - b->bkt[s] > 0);
	}
	b->sk_num = (sk_num_pair *)malloc(sizeof(sk_num_pair) * b->num_nonempty_buckets);
	int num_elements, j = 0;
	for(sketch_type s = 0; s < (1 << PJT_DIM); s++) {
		num_elements = b->bkt[s + 1] - b->bkt[s];
		if(num_elements > 0) {
			b->sk_num[j++] = (sk_num_pair) {s, num_elements, b->bkt[s]};
		}
	}

	return b;
}
#else

#ifndef EXPANDED_SKETCH

int comp_sketch(sketch_type a, sketch_type b)
{
	if(a < b)
		return -1;
	else if(a == b)
		return 0;
	else
		return 1;
}

#else

int comp_sketch(sketch_type a, sketch_type b)
{
	for(int j = 0; j < SKETCH_SIZE; j++) {
		if(a[j] < b[j])
			return -1;
		else if(a[j] == b[j])
			continue;
		else
			return 1;
	}
	return 0;
}

#endif

// WIDE_SKETCH および EXPANDED_SKETCH 用（bkt は大きくなり過ぎるので使用しない）
struct_bucket *new_bucket(int num_data, sketch_type sk[])
// version 2.0 (using arrays only idx and bucket)
{
	struct_bucket *b = (struct_bucket *)malloc(sizeof(struct_bucket));
	b->num_data = num_data;
	b->ftr_data = NULL;
	b->sk = NULL;

	b->idx = (int *)malloc(sizeof(int) * num_data);
	for(int i = 0; i < num_data; i++) {
		b->idx[i] = i;
	}
	for(int i = 0; i < 100; i++) {
		#ifndef EXPANDED_SKETCH
		printf("idx[%d] = %d, sk[idx[%d]] = %lu\n", i, b->idx[i], i, sk[b->idx[i]]);
		#else
		printf("idx[%d] = %d, sk[idx[%d]] = ", i, b->idx[i], i);
		for(int j = 0; j < SKETCH_SIZE; j++) {
			printf("%lu:", sk[b->idx[i]][j]);
		}
		printf("\n");
		#endif
	}
	fprintf(stderr, "sort sketch starts: num_data = %d\n", num_data);
	quick_sort_for_sketch(b->idx, sk, 0, num_data - 1);
	fprintf(stderr, "sort sketch ended: num_data = %d\n", num_data);
	for(int jj = 0; jj < 100; jj++) {
		int i = jj * 100;
		#ifndef EXPANDED_SKETCH
		printf("idx[%d] = %d, sk[idx[%d]] = %lu\n", i, b->idx[i], i, sk[b->idx[i]]);
		#else
		printf("idx[%d] = %d, sk[idx[%d]] = ", i, b->idx[i], i);
		for(int j = 0; j < SKETCH_SIZE; j++) {
			printf("%lu:", sk[b->idx[i]][j]);
		}
		printf("\n");
		#endif
	}

	b->sk_num = (sk_num_pair *)malloc(sizeof(sk_num_pair) * num_data); // 少し大きめに割り当てる
	b->num_nonempty_buckets = 0;
	int i, j;
//	sketch_type s;
	sk_num_pair snp;
	for(i = 0; i < num_data; ) {
		#ifndef EXPANDED_SKETCH
		snp.sk = sk[b->idx[i]];
		#else
		memcpy(snp.sk, sk[b->idx[i]], sizeof(sketch_type));
		#endif
//		for(j = i + 1; j < num_data && s == sk[b->idx[j]]; j++);
		for(j = i + 1; j < num_data && comp_sketch(snp.sk, sk[b->idx[j]]) == 0; j++);
		snp.num = j - i;
		snp.pos = i;
		b->sk_num[b->num_nonempty_buckets++] = snp; //(sk_num_pair) {s, num_elements, i};
//		if(b->num_nonempty_buckets < 100) {
//			printf("num_nonempty_buckets = %d, s = %lu, num = %d, i = %d\n", b->num_nonempty_buckets, s, num_elements, i);
//		}
		i = j;
	}

	return b;
}
#endif

// バケット表（idx と bkt）を含む構造体（struct_bucket）をコンパクトな形式でファイルに書き出す
// bkt は PJT_DIM (width) に対して指数オーダーなので，書き出さない．空でないバケット情報のみ書き出す．
void write_bucket(char *filename, struct_bucket *b)
{
	int num_data = b->num_data, *idx = b->idx, num_nonempty_buckets = b->num_nonempty_buckets;
	sk_num_pair *sk_num = b->sk_num;

	FILE *fp;

	if((fp = fopen(filename, "wb"))  == NULL) {
		fprintf(stderr, "Write open bucket file error, file name = %s\n", filename);
		exit(0);
	}
	if(fwrite(&num_data, sizeof(int), 1, fp) != 1) {  // num_data を書き出す
		fprintf(stderr, "fwrite error (num_data) file = %s\n", filename);
		exit(0);
	}
	if(fwrite(idx, sizeof(int) * num_data, 1, fp) != 1) {  // idx[data_num] を書き出す
		fprintf(stderr, "fwrite error (idx) file = %s\n", filename);
		exit(0);
	}

	if(fwrite(&num_nonempty_buckets, sizeof(int), 1, fp) != 1) {  // num_nonempty_buckets を書き出す
		fprintf(stderr, "fwrite error (num_nonempty_buckets) file = %s\n", filename);
		exit(0);
	}
	fprintf(stderr, "num_nonempty_buckets = %d, average number of elements in nonempty buckets = %.2lf\n", num_nonempty_buckets, (double)num_data / num_nonempty_buckets);
//	sketch_type s;
//	int num_elements;
	for(int i = 0; i < num_nonempty_buckets; i++) {
		#ifndef EXPANDED_SKETCH
		if(fwrite(&sk_num[i].sk, sizeof(sketch_type), 1, fp) != 1) {  // sketch s を書き出す
			fprintf(stderr, "fwrite error (sketch) file = %s\n", filename);
			exit(0);
		}
		#else
		if(fwrite(sk_num[i].sk, sizeof(sketch_type), 1, fp) != 1) {  // sketch s を書き出す
			fprintf(stderr, "fwrite error (sketch) file = %s\n", filename);
			exit(0);
		}
		#endif
		if(fwrite(&sk_num[i].num, sizeof(int), 1, fp) != 1) {  // num_elements を書き出す
			fprintf(stderr, "fwrite error (num_elements) file = %s\n", filename);
			exit(0);
		}
	}
	fclose(fp);
	return;
}

void free_bucket(struct_bucket *b) {
	if(b->ftr_data != NULL) free(b->ftr_data);
	if(b->sk != NULL) free(b->sk);
	if(b->idx != NULL) free(b->idx);
	#ifdef NARROW_SKETCH
	if(b->bkt != NULL) free(b->bkt);
	#endif
	if(b->sk_num != NULL) free(b->sk_num);
	free(b);
}

// コンパクトな形式でファイルに保存されたバケット表を読み込んで（idx と bkt）を含む構造体（struct_bucket）に展開する
struct_bucket *read_bucket(char *filename)
{
	FILE *fp;
	if((fp = fopen(filename, "rb"))  == NULL) {
		fprintf(stderr, "Read open bucket file error, file name = %s\n", filename);
		exit(0);
	}

	int num_data;
	if(fread(&num_data, sizeof(int), 1, fp) != 1) {  // ファイルに書かれている num_data を読み込む
		fprintf(stderr, "fread error (num_data) file = %s\n", filename);
		exit(0);
	}

	struct_bucket *b = (struct_bucket *)malloc(sizeof(struct_bucket));
	b->num_data = num_data;
	b->ftr_data = NULL;  // ここではデータセットは読み込まない．オンメモリ検索をするときには，必要に応じて ftr または sftr を読み込む
	b->idx = (int *)malloc(sizeof(int) * num_data);
	if(fread(b->idx, sizeof(int) * num_data, 1, fp) != 1) {  // idx[num_data] を読み込む
		fprintf(stderr, "fread error (idx, size = %ld) file = %s\n", sizeof(int) * num_data, filename);
		exit(0);
	}

	if(fread(&b->num_nonempty_buckets, sizeof(int), 1, fp) != 1) {  // ファイルに書かれている num_nonempty_buckets を読み込む
		fprintf(stderr, "fread error (data_num) file = %s\n", filename);
		exit(0);
	}

	#ifdef NARROW_SKETCH
	b->bkt = (int *)calloc((1 << PJT_DIM) + 2, sizeof(int));
	#endif
	b->sk_num = (sk_num_pair *)malloc(sizeof(sk_num_pair) * b->num_nonempty_buckets);
	int num, j;
//	sketch_type s, s_next;
	sk_num_pair snp;
	#ifdef NARROW_SKETCH
	int s_next = 0;
	#endif
	num = j = 0;
	for(int i = 0; i < b->num_nonempty_buckets; i++) {
		if(fread(&snp.sk, sizeof(sketch_type), 1, fp) != 1) {  // sketch を読み込む
			fprintf(stderr, "fread error (sketch) file = %s\n", filename);
			exit(0);
		}
		if(fread(&snp.num, sizeof(int), 1, fp) != 1) {  // num_elements を読み込む
			fprintf(stderr, "fread error (sketch) file = %s\n", filename);
			exit(0);
		}
		snp.pos = num;
		b->sk_num[j++] = snp; // (sk_num_pair) {s, num_elements, num};
		#ifdef NARROW_SKETCH
		while(s_next <= snp.sk) {
			b->bkt[s_next++] = num;
		}
		#endif
		num += snp.num;
	}
	#ifdef NARROW_SKETCH
	while(s_next < (1 << PJT_DIM) + 2) {
		b->bkt[s_next++] = num;
	}
	#endif
	fprintf(stderr, "num_nonempty_buckets = %d, average number of elements in nonempty buckets = %.2lf\n", b->num_nonempty_buckets, (double)num_data / b->num_nonempty_buckets);
	fclose(fp);

	b->sk = (sketch_type *)malloc(sizeof(sketch_type) * num_data);
	for(int i = 0; i < b->num_nonempty_buckets; i++) {
		for(int j = b->sk_num[i].pos; j < b->sk_num[i].pos + b->sk_num[i].num; j++) {
//			b->sk[b->idx[j]] = b->sk_num[i].sk;
			memcpy(&b->sk[b->idx[j]], &b->sk_num[i].sk, sizeof(sketch_type));
		}
	}

	return b;
}

// bkt ファイルに保存されているバケットを読み込む．ただし，bkt は，配列に展開しない．
// コンパクト表現 compact_bucket，つまり sk_num_pair（空でないスケッチとデータ数の対＋オフセット）の配列を用いる．
struct_bucket *read_compact_bucket(char *filename)
{
	FILE *fp;
	if((fp = fopen(filename, "rb"))  == NULL) {
		fprintf(stderr, "Read open bucket file error, file name = %s\n", filename);
		exit(0);
	}

	int num_data;
	if(fread(&num_data, sizeof(int), 1, fp) != 1) {  // ファイルに書かれている num_data を読み込む
		fprintf(stderr, "fread error (num_data) file = %s\n", filename);
		exit(0);
	}

	struct_bucket *b = (struct_bucket *)malloc(sizeof(struct_bucket));
	b->num_data = num_data;
	b->ftr_data = NULL;  // ここではデータセットは読み込まない．オンメモリ検索をするときには，必要に応じて ftr または sftr を読み込む
	#ifdef NARROW_SKETCH
	b->bkt = NULL; // bkt は配列に展開しない
	#endif
	b->idx = (int *)malloc(sizeof(int) * num_data);
	if(fread(b->idx, sizeof(int) * num_data, 1, fp) != 1) {  // idx[num_data] を読み込む
		fprintf(stderr, "fread error (idx, size = %ld) file = %s\n", sizeof(int) * num_data, filename);
		exit(0);
	}

	if(fread(&b->num_nonempty_buckets, sizeof(int), 1, fp) != 1) {  // ファイルに書かれている num_nonempty_buckets を読み込む
		fprintf(stderr, "fread error (num_nonempty_buckets) file = %s\n", filename);
		exit(0);
	}

//	b->sk_num = (sk_num_pair *)malloc(b->num_nonempty_buckets * sizeof(sk_num_pair));
	b->sk_num = NULL; // おそらく不要なので，動作確認したら，このメンバを削除できるかも
	b->sk = (sketch_type *)malloc(sizeof(sketch_type) * num_data);
	int offset = 0;
	sk_num_pair snp;
//	sketch_type s;
	for(int i = 0; i < b->num_nonempty_buckets; i++) {
		#ifndef EXPANDED_SKETCH
		if(fread(&snp.sk, sizeof(sketch_type), 1, fp) != 1) {  // sketch を読み込む
			fprintf(stderr, "fread error (sketch) file = %s\n", filename);
			exit(0);
		}
		#else
		if(fread(snp.sk, sizeof(sketch_type), 1, fp) != 1) {  // sketch を読み込む
			fprintf(stderr, "fread error (sketch) file = %s\n", filename);
			exit(0);
		}
		#endif
		if(fread(&snp.num, sizeof(int), 1, fp) != 1) {  // num_elements を読み込む
			fprintf(stderr, "fread error (num_elements) file = %s\n", filename);
			exit(0);
		}
		snp.pos = offset;
//		b->sk_num[i] = snp; // (sk_num_pair) {s, num_elements, offset};
		for(int j = snp.pos; j < snp.pos + snp.num; j++) {
			memcpy(b->sk[b->idx[j]], snp.sk, sizeof(sketch_type));
		}
		offset += snp.num;
	}
	fprintf(stderr, "num_nonempty_buckets = %d, average number of elements in nonempty buckets = %.2lf\n", b->num_nonempty_buckets, (double)num_data / b->num_nonempty_buckets);
	fclose(fp);
/*
	b->sk = (sketch_type *)malloc(sizeof(sketch_type) * num_data);
	for(int i = 0; i < b->num_nonempty_buckets; i++) {
//		printf("# i = %d, pos = %d, num = %d\n", i, b->sk_num[i].pos, b->sk_num[i].num);
		for(int j = b->sk_num[i].pos; j < b->sk_num[i].pos + b->sk_num[i].num; j++) {
//			printf("i = %d, j = %d\n", i, j);
			#ifndef EXPANDED_SKETCH
			b->sk[b->idx[j]] = b->sk_num[i].sk;
			#else
			memcpy(b->sk[b->idx[j]], b->sk_num[i].sk, sizeof(sketch_type));
			#endif
		}
	}
*/
	return b;
}

#if defined(NARROW_SKETCH)

#define PARENT(i) ((i)>>1)
#define LEFT(i)   ((i)<<1)
#define RIGHT(i)  (((i)<<1)+1)

void min_heapify_p(int i, struct_que *que)
{
    int l, r;
    int smallest;

    l = LEFT(i);
	r = RIGHT(i);
    if (l < que->qsize && que->element[l].key < que->element[i].key) smallest = l; else smallest = i;
    if (r < que->qsize && que->element[r].key < que->element[smallest].key) smallest = r;
    if (smallest != i) {
        QUE t = que->element[i]; que->element[i] = que->element[smallest]; que->element[smallest] = t;
        min_heapify_p(smallest, que);
    }
}

int deq_p(QUE *q, struct_que *que)
{
    if (que->qsize == 0) return 0;
    memcpy(q, &(que->element[0]), sizeof(QUE));
	que->element[0] = que->element[--(que->qsize)];
    min_heapify_p(0, que);
    return 1;
}

void enq_p(QUE *q, struct_que *que)
{
    int i, ii;

	i = (que->qsize)++;
    memcpy(&(que->element[i]), q, sizeof(QUE));
	while (i > 0 && que->element[ii = PARENT(i)].key > que->element[i].key) {
        QUE t = que->element[i];
		que->element[i] = que->element[ii]; 
		que->element[ii] = t;
        i = ii;
    }
}

void make_empty_que_c2_n(struct_que_c2_n *que)
{
	que->qsize = 0;
	que->detail_size = 0;
}

int new_que_e2_n(struct_que_c2_n *que)
{
	int i = que->detail_size;
	que->detail_size++;
	return i;
}

void min_heapify_c2_n(int i, struct_que_c2_n *que)
{
	QUE_c2 *element = que->element;
    QUE_c2 t = element[i];

    while(1) {
        int l = LEFT(i);
        if (que->qsize <= l) {
            break;
        }
        if (element[l].key < t.key) {
            int r = RIGHT(i);
            if (r < que->qsize && element[r].key < element[l].key) {
                element[i] = element[r];
                i = r;
            }
            else {
                element[i] = element[l];
                i = l;
            }
        }
        else {
            int r = RIGHT(i);
            if (!(r < que->qsize && element[r].key < t.key)) {
                break;
            }
            element[i] = element[r];
            i = r;
        }
    }
    element[i] = t;
}

int deq_c2_n(QUE_c2 *qe, struct_que_c2_n *que)
{
    if (que->qsize == 0) return 0;
    *qe = que->element[0];
    que->element[0] = que->element[que->qsize - 1];
	--(que->qsize);
    min_heapify_c2_n(0, que);
    return 1;
}

void deq_c2_n_del(struct_que_c2_n *que) // Deq で取り除かなかったものを取り除く 
{
    if (que->qsize == 0) return;
    que->element[0] = que->element[que->qsize - 1];
	--(que->qsize);
    min_heapify_c2_n(0, que);
}

void enq_c2_n(QUE_c2 *qe, struct_que_c2_n *que)
{
	QUE_c2 *element = que->element;
    int i, ii;
	QUE_c2 t;

    i = (que->qsize)++;
    element[i] = *qe;
    while (i > 0 && element[ii = PARENT(i)].key > element[i].key) {
        t = element[i];
        element[i] = element[ii]; 
        element[ii] = t;
        i = ii;
    }
}

void enq_c2_n_after_deq(QUE_c2 *qe, struct_que_c2_n *que)
{
    que->element[0] = *qe;
    min_heapify_c2_n(0, que);
}
#endif // NARROW_SKETCH

// 質問のスケッチを作成し，score計算のための分割境界との最小距離や表関数の配列を設定する
struct_query_sketch *make_query_sketch(struct_dataset *ds_query, pivot_type *pivot)
{
	struct_query_sketch *qs = (struct_query_sketch *)calloc(ds_query->num_data, sizeof(struct_query_sketch));
	for(int i = 0; i < ds_query->num_data; i++) {
		qs[i].query = (query_type) {i, ds_query->ftr_id[i].ftr}; //, ds_query->ftr_id[i].data_id;
		// 距離計算の片方が質問に固定されるのでセットする．PQBPでも，全部の部分空間をセットするので，PART_SET_DISTではなくSET_DISTにする
		SET_DIST(ds_query->ftr_id[i].ftr);
		for(int j = 0; j < PJT_DIM; j++) qs[i].idx[j] = j;
		for(int j = 0; j < PJT_DIM; j++) {

			#ifndef PARTITION_TYPE_PQBP
			dist_type dist = DISTANCE_22(pivot->p[j]);
			#else
			dist_type dist = PART_DISTANCE_22(pivot->p[j], PART_START(j), PART_DIM(j));
			#endif

			#ifndef EXPANDED_SKETCH
			WRITE_BIT(j, dist < pivot->r[j], &qs[i].sketch);
			#else
			WRITE_BIT(j, dist < pivot->r[j], qs[i].sketch);
			#endif

			#ifdef SCORE_2
			qs->bd[j] = abs(sqrt(dist) - sqrt(pivot->r[j])) * abs(sqrt(dist) - sqrt(pivot->r[j])); // score_2
			#else
			qs->bd[j] = abs(sqrt(dist) - sqrt(pivot->r[j])); // score_1
			#endif

			// bd をソート（idxを用いた相対ソート）（挿入法：bd[j] を挿入）
			int l;
			for(l = j - 1; l >= 0 && qs[i].bd[qs[i].idx[l]] > qs[i].bd[j]; l--) {
				qs[i].idx[l + 1] = qs[i].idx[l];
			}
			qs[i].idx[l + 1] = j;

			// 表関数の更新（bd[j] を対応するところに加える）
			int tn = j / 8, bp = j % 8; // 表関数の変更すべき表番号が tn，そのビット位置が bp
			for(int n = 0; n < 256; n++) {
				if(n & (1 << bp)) {
					qs[i].tbl[tn][n] += qs[i].bd[j];
				}
			}
		}
	}
	return qs;
}

void set_query_sketch(struct_query_sketch *qs, query_type *query, pivot_type *pivot)
{
	qs->query = *query;
	#if defined(NARROW_SKETCH)
	qs->sketch = 0;
	int tbl_size = 4;
	#elif defined(WIDE_SKETCH)
	qs->sketch = 0;
	int tbl_size = 8;
	#else
	for(int i = 0; i < SKETCH_SIZE; i++) qs->sketch[i] = 0;
	int tbl_size = TABLE_SIZE;
	#endif
	for(int p = 0; p < tbl_size; p++) {
		for(int n = 0; n < 256; n++) {
			qs->tbl[p][n] = 0;
		}
	}
	for(int j = 0; j < PJT_DIM; j++) qs->idx[j] = j;
	for(int j = 0; j < PJT_DIM; j++) {
		#ifndef PARTITION_TYPE_PQBP
		dist_type dist = DISTANCE(query->ftr, pivot->p[j], FTR_DIM);
		#else
		dist_type dist = PART_DISTANCE(query->ftr, pivot->p[j], PART_START(j), PART_DIM(j));
		#endif
		#ifndef EXPANDED_SKETCH
		WRITE_BIT(j, dist < pivot->r[j], &qs->sketch);
		#else
		WRITE_BIT(j, dist < pivot->r[j], qs->sketch);
		#endif
		#ifdef SCORE_2
		qs->bd[j] = abs(sqrt(dist) - sqrt(pivot->r[j])) * abs(sqrt(dist) - sqrt(pivot->r[j])); // score_2
		#else
		qs->bd[j] = abs(sqrt(dist) - sqrt(pivot->r[j])); // score_1
		#endif

		// bd をソート（idxを用いた相対ソート）（挿入法：bd[j] を挿入）
		int l;
		for(l = j - 1; l >= 0 && qs->bd[qs->idx[l]] > qs->bd[j]; l--) {
			qs->idx[l + 1] = qs->idx[l];
		}
		qs->idx[l + 1] = j;

		// 表関数の更新（bd[j] を対応するところに加える）
		int tn = j / 8, bp = j % 8; // 表関数の変更すべき表番号が tn，そのビット位置が bp
		for(int n = 0; n < 256; n++) {
			if(n & (1 << bp)) {
				qs->tbl[tn][n] += qs->bd[j];
			}
		}
	}
}

// for score_p
void set_query_sketch_p(struct_query_sketch *qs, query_type *query, pivot_type *pivot, double p)
{
	qs->query = *query;
	#if defined(NARROW_SKETCH)
	qs->sketch = 0;
	int tbl_size = 4;
	#elif defined(WIDE_SKETCH)
	qs->sketch = 0;
	int tbl_size = 8;
	#else
	for(int i = 0; i < SKETCH_SIZE; i++) qs->sketch[i] = 0;
	int tbl_size = TABLE_SIZE;
	#endif
	for(int p = 0; p < tbl_size; p++) {
		for(int n = 0; n < 256; n++) {
			qs->tbl[p][n] = 0;
		}
	}
	for(int j = 0; j < PJT_DIM; j++) qs->idx[j] = j;
	for(int j = 0; j < PJT_DIM; j++) {
		#ifndef PARTITION_TYPE_PQBP
		dist_type dist = DISTANCE(query->ftr, pivot->p[j], FTR_DIM);
		#else
		dist_type dist = PART_DISTANCE(query->ftr, pivot->p[j], PART_START(j), PART_DIM(j));
		#endif
		#ifndef EXPANDED_SKETCH
		WRITE_BIT(j, dist < pivot->r[j], &qs->sketch);
		#else
		WRITE_BIT(j, dist < pivot->r[j], qs->sketch);
		#endif
		qs->bd[j] = pow(abs(sqrt(dist) - sqrt(pivot->r[j])), p) ; // score_p

		// bd をソート（idxを用いた相対ソート）（挿入法：bd[j] を挿入）
		int l;
		for(l = j - 1; l >= 0 && qs->bd[qs->idx[l]] > qs->bd[j]; l--) {
			qs->idx[l + 1] = qs->idx[l];
		}
		qs->idx[l + 1] = j;

		// 表関数の更新（bd[j] を対応するところに加える）
		int tn = j / 8, bp = j % 8; // 表関数の変更すべき表番号が tn，そのビット位置が bp
		for(int n = 0; n < 256; n++) {
			if(n & (1 << bp)) {
				qs->tbl[tn][n] += qs->bd[j];
			}
		}
	}
}

dist_type priority(sketch_type s, struct_query_sketch *q)
{
#ifndef SCORE_INF // score_1 or score_2
#ifndef EXPANDED_SKETCH
	sketch_type d = s ^ q->sketch;
	#if defined(NARROW_SKETCH)
	return q->tbl[0][d & 0xff] + q->tbl[1][(d >> 8) & 0xff] + q->tbl[2][(d >> 16) & 0xff] + q->tbl[3][d >> 24];
	#else
	return q->tbl[0][d & 0xff] + q->tbl[1][(d >> 8) & 0xff] + q->tbl[2][(d >> 16) & 0xff] + q->tbl[3][(d >> 24) & 0xff]
	     + q->tbl[4][(d >> 32) & 0xff] + q->tbl[5][(d >> 40) & 0xff] + q->tbl[6][(d >> 48) & 0xff] + q->tbl[7][d >> 56];
	#endif
#else
	dist_type p = 0;
	unsigned long d;
	for(int i = 0; i < SKETCH_SIZE; i++) {
		d = s[i] ^ q->sketch[i];
		p += q->tbl[i * 8 + 0][d & 0xff] + q->tbl[i * 8 + 1][(d >> 8) & 0xff] + q->tbl[i * 8 + 2][(d >> 16) & 0xff] + q->tbl[i * 8 + 3][(d >> 24) & 0xff]
		     + q->tbl[i * 8 + 4][(d >> 32) & 0xff] + q->tbl[i * 8 + 5][(d >> 40) & 0xff] + q->tbl[i * 8 + 6][(d >> 48) & 0xff] + q->tbl[i * 8 + 7][d >> 56];
	}
	return p;
#endif
#else // score_inf
#ifndef EXPANDED_SKETCH
	sketch_type d = s ^ q->sketch;
	dist_type inf = 0;

	inf = q->tbl[0][d & 0xff];
	inf = inf >= q->tbl[1][(d >>  8) & 0xff] ? inf : q->tbl[1][(d >> 8 ) & 0xff];
	inf = inf >= q->tbl[2][(d >> 16) & 0xff] ? inf : q->tbl[2][(d >> 16) & 0xff];
	inf = inf >= q->tbl[3][(d >> 24) & 0xff] ? inf : q->tbl[3][(d >> 24) & 0xff];
	#ifdef WIDE_SKETCH
	inf = inf >= q->tbl[4][(d >> 32) & 0xff] ? inf : q->tbl[4][(d >> 32) & 0xff];
	inf = inf >= q->tbl[5][(d >> 40) & 0xff] ? inf : q->tbl[5][(d >> 40) & 0xff];
	inf = inf >= q->tbl[6][(d >> 48) & 0xff] ? inf : q->tbl[6][(d >> 48) & 0xff];
	inf = inf >= q->tbl[7][ d >> 56        ] ? inf : q->tbl[7][ d >> 56        ];
	#endif
	return inf;
#else
	dist_type p = 0, inf;
	unsigned long d;
	for(int i = 0; i < SKETCH_SIZE; i++) {
		d = s[i] ^ q->sketch[i];
		inf = q->tbl[0][d & 0xff];
		inf = inf >= q->tbl[1][(d >>  8) & 0xff] ? inf : q->tbl[1][(d >> 8 ) & 0xff];
		inf = inf >= q->tbl[2][(d >> 16) & 0xff] ? inf : q->tbl[2][(d >> 16) & 0xff];
		inf = inf >= q->tbl[3][(d >> 24) & 0xff] ? inf : q->tbl[3][(d >> 24) & 0xff];
		inf = inf >= q->tbl[4][(d >> 32) & 0xff] ? inf : q->tbl[4][(d >> 32) & 0xff];
		inf = inf >= q->tbl[5][(d >> 40) & 0xff] ? inf : q->tbl[5][(d >> 40) & 0xff];
		inf = inf >= q->tbl[6][(d >> 48) & 0xff] ? inf : q->tbl[6][(d >> 48) & 0xff];
		inf = inf >= q->tbl[7][ d >> 56        ] ? inf : q->tbl[7][ d >> 56        ];
		p = p >= inf ? p : inf;
	}
	return p;
#endif
#endif	
}

dist_type hamming(sketch_type s, sketch_type t)
{
#ifndef EXPANDED_SKETCH
	sketch_type d = s ^ t;
	#if defined(NARROW_SKETCH)
	return bit_count(d);
	#else
	return bit_count_long(d);
	#endif
#else
	dist_type h = 0;
	unsigned long d;
	for(int i = 0; i < SKETCH_SIZE; i++) {
		d = s[i] ^ t[i];
		h += bit_count_long(d);
	}
	return h;
#endif
}

void filtering_by_sequential_search_using_kNN_buffer(struct_query_sketch *qs, int num_data, sketch_type sketch[], kNN_buffer *buff, int data_num[], int num_candidates)
{
	static answer_type *ans = NULL;
	if(ans == NULL) ans = (answer_type *)malloc(sizeof(answer_type) * num_data);
	// 優先順位（priority）の計算だけ並列処理する（kNN_buffer による選択はフィルタリングでは効果がないので，直列処理する）
	#ifdef _OPENMP
	#if defined(NUM_THREADS) && NUM_THREADS > 1
	omp_set_num_threads(NUM_THREADS);
	#pragma omp parallel for
	#endif
	#endif
	for(int i = 0; i < num_data; i++) {
		ans[i].data_num = i;
		ans[i].dist = priority(sketch[i], qs);
	}
	if(num_candidates == 0) {
		return; // scoring (calculation of priorities) only
	}
	dist_type k_nearest = make_empty_kNN_buffer(buff);
	for(int i = 0; i < num_data; i++) {
		if(ans[i].dist < k_nearest) k_nearest = push_kNN_buffer(&ans[i], buff);
	}
	k_nearest = flush_kNN_buffer(buff);
	for(int i = 0; i < num_candidates; i++) {
		data_num[i] = buff->buff[i].data_num;
	}
}

void filtering_by_sequential_search_using_quick_select_k(struct_query_sketch *qs, int num_data, sketch_type sketch[], dist_type score[], int data_num[], int num_candidates)
{
	#ifdef _OPENMP
	#if defined(NUM_THREADS) && NUM_THREADS > 1
	omp_set_num_threads(NUM_THREADS);
	#pragma omp parallel for
	#endif
	#endif
	for(int i = 0; i < num_data; i++) {
		data_num[i] = i;
		score[i] = priority(sketch[i], qs);
	}
	if(num_candidates == 0) {
		return; // scoring (calculation of priorities) only
	}
	#ifdef QUICK_SELECT
	quick_select_k(data_num, score, 0, num_data - 1, num_candidates); 
	#else
	quick_sort(data_num, score, 0, num_data - 1); 
	#endif
}

#if defined(NARROW_SKETCH)
// スケッチ列挙によるフィルタリング（バケット（配列 idx と bkt）利用）
void filtering_by_sketch_enumeration(struct_query_sketch *qs, struct_bucket *bucket, struct_que *que, int data_num[], int num_candidates)
{
	sketch_type s;
	QUE qu, qu2;
	int *bd = qs->bd, *bd_idx = qs->idx;
	int *bkt = bucket->bkt;

	s = qs->sketch;
	int k = 0;
	for(int j = bkt[s]; j < bkt[s + 1] && k < num_candidates; j++, k++) {
		data_num[k] = j;
	}
	que->qsize = 0;
	qu.key = bd[bd_idx[0]];
	qu.sk = s ^ (1 << bd_idx[0]);
	qu.idx = 1;
	enq_p(&qu, que);

	while(deq_p(&qu, que) && k < num_candidates) {
		for(int j = bkt[qu.sk]; j < bkt[qu.sk + 1] && k < num_candidates; j++, k++) {
			data_num[k] = j;
		}
		if(qu.idx < PJT_DIM) {
			s = qu.sk ^ (1 << bd_idx[qu.idx]);
			qu2.key = qu.key + bd[bd_idx[qu.idx]];
			qu2.sk = s;
			qu2.idx = qu.idx + 1;
			enq_p(&qu2, que);

			qu2.key = qu.key + bd[bd_idx[qu.idx]] - bd[bd_idx[qu.idx - 1]];
			qu2.sk = s ^ (1 << bd_idx[qu.idx - 1]);
			qu2.idx = qu.idx + 1;
			enq_p(&qu2, que);
		}
	}
}

void filtering_by_sketch_enumeration_c2_n(struct_query_sketch *qs, struct_bucket *bucket, struct_que_c2_n *que, int data_num[], int num_candidates)
{
	sketch_type s;
	QUE_c2 qu, qu2;
	int *bd = qs->bd, *bd_idx = qs->idx;
	int *bkt = bucket->bkt;
	
	s = qs->sketch;
	int k = 0;
	for(int j = bkt[s]; j < bkt[s + 1] && k < num_candidates; j++, k++) {
		data_num[k] = j;
	}
	s = s ^ (1 <<  bd_idx[0]);
	for(int j = bkt[s]; j < bkt[s + 1] && k < num_candidates; j++, k++) {
		data_num[k] = j;
	}
	make_empty_que_c2_n(que);

	// enq pattern of 0...10
	qu.cursor = new_que_e2_n(que);
	qu.key = bd[bd_idx[1]];
	que->details[qu.cursor].sk = qs->sketch ^ (1 << bd_idx[1]);
	que->details[qu.cursor].pt = 1 << 1; // pt = "0...00000010"
	enq_c2_n(&qu, que);		

	while(deq_c2_n(&qu, que) && k < num_candidates) {
		s = que->details[qu.cursor].sk;
		for(int j = bkt[s]; j < bkt[s + 1] && k < num_candidates; j++, k++) {
			data_num[k] = j;
		}

		switch(que->details[qu.cursor].pt & 15) {
		case 0: // X0000 -> enq(X0001) and enq(Y10^{m+1}) if X0000 = Y010^m
		case 8: // X1000 -> enq(X1001) and enq(Y10^{m+1}) if X0000 = Y010^m
			{
				int m = lsb_pos(que->details[qu.cursor].pt);
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
}
#endif
/*
バケットの表現

0.	前提
	int data_num; // データ数
	ftr[0], ... , ftr[data_num - 1]; // 特徴データ（オリジナル）
	sk[0], ... , sk[data_num - 1]; // スケッチ
	sftr[0], ... , sftr[data_num - 1]; // 特徴データをスケッチ順に並べたもの
	idx[i] = "sftr[i] のオリジナルのインデックス" （sftr[i] = ftr[idx[i]]）
			「スケッチ順で i 番目のデータがオリジナルで何番目であるかと表す」
	bkt[s] = スケッチ順にデータを並べたときに，スケッチが s であるものの先頭の位置 (0 - data_num - 1)
	bkt[s + 1] - bkt[s] = スケッチが s であるもののデータ数

	スケッチが s であるデータ = sftr[j] (j = bkt[s], ... , bkt[s + 1] - 1) (sftr が使えるとき)
	スケッチが s であるデータ = ftr[idx[j]] (j = bkt[s], ... , bkt[s + 1] - 1) 
	スケッチが s であるデータのオリジナルインデックス = idx[j] (j = bkt[s], ... , bkt[s + 1] - 1)
   	
1. 	プログラム内部

	オプション1 (FTR_ON): 特徴データを主記憶に置くかどうか (FTR_ON = MAIN_MEMORY | SECONDARY_MEMORY)
	オプション2 (FTR_ARRANGEMANT): 特徴データをスケッチ順にソートしたものを用いるかどうか (FTR_ARRANGEMENT = SORT_BY_SKETCH | ASIS)
	オプション3 (FILTERING_BY): フィルタリング手法 (FILTERING_BY = SKETCH_ENUMERATION | SEQUENTIAL_SEARCH | SEQUENTIAL_SEARCH_USING_BUCKET)

1.1	オンメモリ（すべて主記憶上に読み込んでいる）
	FTR_ON = MAIN_MEMORY, FTR_ARRANGEMANT = SORT_BY_SKETCH | ASIS, FILTERING_BY = SKETCH_ENUMERATION
	database[data_num]; 			// 特徴データ（オリジナル）
	sketch[data_num];  				// データのスケッチ
	bkt[2^w + 2], idx[data_num];	// バケット表とデータのインデックス
	database2[data_num]; 			// スケッチ順の特徴データ

1.2	特徴データを2次記憶（HDD，SSD）に置いたまま
	FTR_ON = SECONDARY_MEMORY, FTR_ARRANGEMANT = SORT_BY_SKETCH | ASIS, FILTERING_BY = SKETCH_ENUMERATION
	bkt[2^w + 2], idx[data_num];	// バケット表とデータのインデックス
	特徴データは，sftrを2次記憶上に置いておく．
	検索は，スケッチ列挙法による．

1.3	Sequential Filtering （オンメモリ）
	FTR_ON = MAIN_MEMORY, FTR_ARRANGEMANT = ASIS, FILTERING_BY = SEQUENTIAL_SEARCH
	database[data_num]; 			// 特徴データ（オリジナル）
	sketch[data_num];  				// データのスケッチ

1.4	Sequential Filtering using bucket
	FTR_ON = SECONDARY_MEMORY, FTR_ARRANGEMANT = SORT_BY_SKETCH | ASIS, FILTERING_BY = SEQUENTIAL_SEARCH_USING_BUCKET
	database[data_num]; 			// 特徴データ（オリジナル）
	bkt[2^w + 2], idx[data_num];	// バケット表とデータのインデックス

		
2. 外部ファイルの保存時

*/

#ifndef EXPANDED_SKETCH
// スケッチの配列を相対的にソートする．idxを入れ替える．
int find_pivot_for_sketch(int idx[], sketch_type sk[], int i, int j)
{
   int k;

	for(int t = 0; t < TRIAL; t++) {
		k = random() % (j - i + 1) + i;
		if(sk[idx[i]] != sk[idx[k]]) {
			return (sk[idx[i]] > sk[idx[k]] ? i : k);
		}
	}
	for(k = i + 1; k <= j; k++) {
		if(sk[idx[i]] != sk[idx[k]]) {
			return (sk[idx[i]] > sk[idx[k]] ? i : k);
		}
	}
	return -1;
}

int partition_by_pivot_for_sketch(int idx[], sketch_type sk[], int i, int j, sketch_type piv)
{
   int left, right;
   int temp;
   left = i;   right = j;
   do {
      while(sk[idx[left]] < piv) left++;
      while(sk[idx[right]] >= piv) right--;
      if (left < right) { 
         temp = idx[left];
         idx[left] = idx[right];
         idx[right] = temp;
      }
   } while(left <= right);
   return left;
}

void quick_sort_for_sketch(int idx[], sketch_type sk[], int i, int j)
{
	int pivotindex, k;
	pivotindex = find_pivot_for_sketch(idx, sk, i, j);
	if (pivotindex >= 0) {
		k = partition_by_pivot_for_sketch(idx, sk, i, j, sk[idx[pivotindex]]);
		quick_sort_for_sketch(idx, sk, i, k - 1);
		quick_sort_for_sketch(idx, sk, k, j);
	}
}
#else

//static int comp_sketch(sketch_type a, sketch_type b)
//{
//	for(int j = 0; j < SKETCH_SIZE; j++) {
//		if(a[j] < b[j])
//			return -1;
//		else if(a[j] == b[j])
//			continue;
//		else
//			return 1;
//	}
//	return 0;
//}

// スケッチの配列を相対的にソートする．idxを入れ替える．
#ifndef TRIAL
#define TRIAL 5
#endif
int find_pivot_for_sketch(int idx[], sketch_type sk[], int i, int j)
{
	int k, cmp;

	for(int t = 0; t < TRIAL; t++) {
		k = random() % (j - i + 1) + i;
		if((cmp = comp_sketch(sk[idx[i]], sk[idx[k]])) != 0) {
			return (cmp > 0 ? i : k);
		}
	}
	for(k = i + 1; k <= j; k++) {
		if((cmp = comp_sketch(sk[idx[i]], sk[idx[k]])) != 0) {
			return (cmp > 0 ? i : k);
		}
	}
	return -1;
}

int partition_by_pivot_for_sketch(int idx[], sketch_type sk[], int i, int j, sketch_type piv)
{
	int left, right;
	int temp;
	left = i;   right = j;
	do {
		while(comp_sketch(sk[idx[left]], piv) < 0) left++;
		while(comp_sketch(sk[idx[right]], piv) >= 0) right--;
		if (left < right) { 
			temp = idx[left];
			idx[left] = idx[right];
			idx[right] = temp;
		}
	} while(left <= right);
	return left;
}

void quick_sort_for_sketch(int idx[], sketch_type sk[], int i, int j)
{
	int pivotindex, k;
	pivotindex = find_pivot_for_sketch(idx, sk, i, j);
	if (pivotindex >= 0) {
		k = partition_by_pivot_for_sketch(idx, sk, i, j, sk[idx[pivotindex]]);
		quick_sort_for_sketch(idx, sk, i, k - 1);
		quick_sort_for_sketch(idx, sk, k, j);
	}
}
#endif

