#include "kNN_search.h"
#include "pivot_selection.h"
#include "sketch.h"
#include "quick.h"
#include <omp.h>

ftr_type *get_sample(struct_dataset *ds)
{
	int sample_number[SAMPLE_SIZE];
	ftr_type *sample = (ftr_type *)malloc(sizeof(ftr_type) * SAMPLE_SIZE);
	for(int i = 0; i < SAMPLE_SIZE;){
		int x = random() % ds->num_data;
		int j;
		for(j = 0; j < i; j++) {
			if(x == sample_number[j]) break; // 同じデータは使わないようにする
		}
		if(j < i) continue;
/*
		for(j = 0; j < i; j++) {
			if(DISTANCE(ds->ftr_id[x].ftr, sample[j], FTR_DIM) == 0) break;
		}
		if(j < i) {
			fprintf(stderr, "sample[%d] from ds[%d] = ds[%d]\n", j, sample_number[j], x);
			continue;
		}
*/
		sample[i] = ds->ftr_id[x].ftr;
		sample_number[i++] = x;
	}
	return sample;
}

ftr_type get_median(struct_dataset *ds)
{
	int cnt[256];
	int i, j, s, t;
	ftr_type med = (ftr_type)malloc(FTR_DIM * sizeof(ftr_element_type));

	for(j = 0; j < FTR_DIM; j++) {
		for(i = 0; i < 256; i++) {
			cnt[i] = 0;
		}
		
		for(i = t = 0; i < ds->num_data; i += 500) {
			cnt[ds->ftr_id[i].ftr[j]]++; t++;
		}
		for(i = s = 0; (i < 256) && (s < t / 2); i++) {
			s += cnt[i];
		}
		med[j] = i;
	}
	return med;
}

#ifndef NUM_TRIAL_QBP
#define NUM_TRIAL_QBP 1
#endif

void select_pivot_QBP(pivot_type *pivot, ftr_type median, ftr_type sample[], struct_dataset *ds, int nt)
{
// とりあえず，乱択したデータを2値量子化した点を用いてピボットを作る．
// 最小衝突法は，素朴にサンプルの衝突を計算する版を用いる．高速化版は後回し．
	static dist_type *sample_dist = NULL;
	static int *idx = NULL;
	static sketch_type *sample_sketch = NULL;
	if(sample_dist == NULL)
		sample_dist = (dist_type *)malloc(sizeof(dist_type) * SAMPLE_SIZE);
	if(idx == NULL)
		idx = (int *)malloc(sizeof(int) * SAMPLE_SIZE);
	if(sample_sketch == NULL)
		sample_sketch = (sketch_type *)malloc(sizeof(sketch_type) * SAMPLE_SIZE);
	dist_type min_r = 0;				// それまでに求めた最良のピボットの半径
	ftr_element_type min_p[FTR_DIM];	// 								 のセンター点

	#ifdef _OPENMP
	#pragma omp parallel for
	#endif
	for(int i = 0; i < SAMPLE_SIZE; i++) {
		#ifndef EXPANDED_SKETCH
		sample_sketch[i] = 0;
		#else
		for(int j = 0; j < SKETCH_SIZE; j++) {
			sample_sketch[i][j] = 0;
		}
		#endif
	}
	
	// QBP で使用する乱数がget_threshold_kやquick_select内部で使用する乱数と干渉するのを避けるために，ここでまとめて乱数を取得しておく
	int rdm[NUM_TRIAL_QBP * PJT_DIM];
	for(int i = 0; i < NUM_TRIAL_QBP * PJT_DIM; i++) rdm[i] = random() % ds->num_data;
	int ir = 0;

	int dim;
	double scmin;
	for(dim = 0; dim < PJT_DIM; dim++) {
		scmin = DBL_MAX;
		for(int t = 0; t < NUM_TRIAL_QBP; t++){	// 試行回数
//			int c = random() % ds->num_data;
			int c = rdm[ir++];
			#ifdef _OPENMP
			#pragma omp parallel for
			#endif
			for(int i = 0; i < FTR_DIM; i++) {
				#ifndef PARTITION_TYPE_PQBP
				pivot->p[dim][i] = ds->ftr_id[c].ftr[i] < median[i] ? 0 : 255;
				#else
				if(i < PART_START(dim) || i >= PART_START(dim) + PART_DIM(dim)) {
					pivot->p[dim][i] = 127; // 使用しないところはすべて0にしておく．
				} else {
					pivot->p[dim][i] = ds->ftr_id[c].ftr[i] < median[i] ? 0 : 255;
				}
			#endif
			}
#ifndef SAMPLE_SIZE_FOR_RADIUS
#define SAMPLE_SIZE_FOR_RADIUS SAMPLE_SIZE
#endif
			int ss = SAMPLE_SIZE_FOR_RADIUS;
			#ifdef _OPENMP
			#pragma omp parallel for
			#endif
			for(int i = 0; i < ss; i++) {
				int j = random() % ss;
				#ifndef PARTITION_TYPE_PQBP
				sample_dist[i] = DISTANCE(pivot->p[dim], sample[j], FTR_DIM);
				#else
				sample_dist[i] = PART_DISTANCE(pivot->p[dim], sample[j], PART_START(dim), PART_DIM(dim));
				#endif
				idx[i] = i;
			}
			if(ss > 1000) {
				pivot->r[dim] = get_threshold_k(idx, sample_dist, ss, ss / 2, nt);  // 半径計算
//				pivot->r[dim] = get_threshold_k(idx, sample_dist, ss, 3 * ss / 9 + random() % (2 * ss / 9), nt);  // 半径計算
			} else {
				insertion_sort(sample_dist, ss);
				pivot->r[dim] = sample_dist[ss / 2];
			}
			#ifdef _OPENMP
			#pragma omp parallel for
			#endif
			for(int i = 0; i < SAMPLE_SIZE; i++) {
				#ifndef EXPANDED_SKETCH
				data_to_sketch_1bit(sample[i], pivot, dim, &sample_sketch[i]);
				#else
				data_to_sketch_1bit(sample[i], pivot, dim, sample_sketch[i]);
//				if(dim == 3 && i < 10) {
//					printf("i = %2d:", i); print_bin_expanded(sample_sketch[i], SKETCH_SIZE); printf("\n"); getchar();
//				}
				#endif
			}
			double sc = collision(sample_sketch);
			if(sc < scmin) {
				#ifdef _OPENMP
				#pragma omp parallel for
				#endif
				for(int i = 0; i < FTR_DIM; i++) {
					min_p[i] = pivot->p[dim][i];
				}
				scmin = sc;
				min_r = pivot->r[dim];
			}
		}

		#ifdef _OPENMP
		#pragma omp parallel for
		#endif
		for(int i = 0; i < FTR_DIM; i++) {
			pivot->p[dim][i] = min_p[i]; //best pivot
		}
		pivot->r[dim] = min_r;           //best radian
		fprintf(stderr,"DIM%3d fin ... = %12.0f\n", dim, scmin);
		if(scmin <= 1.0) {
			fprintf(stderr, "replace sample\n");
			free(sample);
			sample = get_sample(ds);
			#ifdef _OPENMP
			#pragma omp parallel for
			#endif
			for(int i = 0; i < SAMPLE_SIZE; i++) {  // 新しいsampleのsketchを作り直す
				for(int j = 0; j <= dim; j++) {
					#ifndef EXPANDED_SKETCH
					data_to_sketch_1bit(sample[i], pivot, j, &sample_sketch[i]);
					#else
					data_to_sketch_1bit(sample[i], pivot, j, sample_sketch[i]);
					#endif
				}
			}
		} else {
			#ifdef _OPENMP
			#pragma omp parallel for
			#endif
			for(int i = 0; i < SAMPLE_SIZE; i++) { // Bestなpivotでsketchを作り直す（Trial中にsketchを書き換えているため）
				#ifndef EXPANDED_SKETCH
				data_to_sketch_1bit(sample[i], pivot, dim, &sample_sketch[i]);
				#else
				data_to_sketch_1bit(sample[i], pivot, dim, sample_sketch[i]);
				#endif
			}
		}
	}
	fprintf(stderr,"DIM%3d fin ... = %12.0f, %12.4e\n", dim, scmin, scmin / ((double)SAMPLE_SIZE * (SAMPLE_SIZE - 1) / 2));

	fprintf(stderr, "XXX\n");
	// サンプルデータの先頭分の100個のスケッチを作って，csv形式で書き出す（stdout）（EXPANDEDは未対応）
/*
	#ifndef EXPANDED_SKETCH
	sketch_type sk;
	for(int i = 0; i < 100; i++) {
		printf("i = %d\n", i);
		for(int j = 0; j < PJT_DIM; j++) {
			data_to_sketch_1bit(ds->ftr_id[i].ftr, pivot, j, &sk);
		}
		for(int j = 0; j < FTR_DIM; j++) {
			printf("%4d,", ds->ftr_id[i].ftr[j]);
		}
		printf("\n%lu,'", (long)sk);
		print_bin_long((long)sk);
		printf("\n");
	}
	#endif
*/
}

void select_pivot_random_QBP(pivot_type *pivot, ftr_type median, struct_dataset *ds)
{
// とりあえず，乱択したデータを2値量子化した点を用いてピボットを作る．
// 最小衝突法は，用いない．
// 半径を求めるときにサンプル数を少なくして，毎回サンプルを選び直して，中央値を求める．
// サンプルが少ないと，中央値はばらつくので，そのばらつきを利用して，アンバランスな分割も混ざるようにする．
	static dist_type sample_dist[SAMPLE_SIZE];

	// QBP で使用する乱数がget_threshold_kやquick_select内部で使用する乱数と干渉するのを避けるために，ここでまとめて乱数を取得しておく
	int rdm[PJT_DIM];
	for(int i = 0; i < PJT_DIM; i++) rdm[i] = random() % ds->num_data;

	int dim;
	for(dim = 0; dim < PJT_DIM; dim++) {
		int c = rdm[dim];
		for(int i = 0; i < FTR_DIM; i++) {
			#ifndef PARTITION_TYPE_PQBP
			pivot->p[dim][i] = ds->ftr_id[c].ftr[i] < median[i] ? 0 : 255;
			#else
			if(i < PART_START(dim) || i >= PART_START(dim) + PART_DIM(dim)) {
				pivot->p[dim][i] = 0; // 使用しないところはすべて0にしておく．
			} else {
				pivot->p[dim][i] = ds->ftr_id[c].ftr[i] < median[i] ? 0 : 255;
			}
			#endif
		}
		// 半径計算
		for(int i = 0; i < SAMPLE_SIZE; i++) {
			int x = random() % ds->num_data;
			#ifndef PARTITION_TYPE_PQBP
			sample_dist[i] = DISTANCE(pivot->p[dim], ds->ftr_id[x].ftr, FTR_DIM);
			#else
			sample_dist[i] = PART_DISTANCE(pivot->p[dim], ds->ftr_id[x].ftr, PART_START(dim), PART_DIM(dim));
			#endif
		}
		insertion_sort(sample_dist, SAMPLE_SIZE);
		pivot->r[dim] = sample_dist[random() % SAMPLE_SIZE]; 
	}
}

#ifndef EXPANDED_SKETCH

static int comp_sketch_2(const void *a, const void *b) {

	if(*(sketch_type *)a < *(sketch_type *)b)
		return -1;
	else if(*(sketch_type *)a == *(sketch_type *)b)
		return 0;
	else
		return 1;
	
}

#else

static int comp_sketch_2(const void *a, const void *b) 
{
	sketch_type *x = (sketch_type *)a, *y = (sketch_type *)b;
	for(int j = 0; j < SKETCH_SIZE; j++) {
		if((*x)[j] < (*y)[j])
			return -1;
		else if((*x)[j] == (*y)[j])
			continue;
		else
			return 1;
	}
	return 0;
}

#endif

double collision(sketch_type sk[])
{
	double n;       // 衝突している組の個数
 	double score = 0;       // スコア
	static sketch_type *temp = NULL;
	if(temp == NULL) {
		temp = (sketch_type *)malloc(sizeof(sketch_type) * SAMPLE_SIZE);
	}
	for(int i = 0; i < SAMPLE_SIZE; i++) {
		#ifndef EXPANDED_SKETCH
		temp[i] = sk[i];
		#else
		for(int j = 0; j < SKETCH_SIZE; j++) {
			temp[i][j] = sk[i][j];
		}
		#endif
	}
	qsort(temp, SAMPLE_SIZE, sizeof(sketch_type), comp_sketch_2); // skのコピーをソート
//	for(int i = 0; i < 10; i++) {
//		printf("b = %2d:", i);
//		print_bin_expanded(sk[i], SKETCH_SIZE);
//		printf("\n");
//	}
//	for(int i = 0; i < 10; i++) {
//		printf("a = %2d:", i);
//		print_bin_expanded(temp[i], SKETCH_SIZE);
//		printf("\n");
//	}
//	getchar();
	for (int i = 0; i < SAMPLE_SIZE - 1; i++) {
		n = 0;
		#ifdef EXPANDED_SKETCH
		while(i < SAMPLE_SIZE - 1 && comp_sketch_2(&temp[i], &temp[i + 1]) == 0) {
			n++;
			i++;
		}
		#else
		while(i < SAMPLE_SIZE - 1 && temp[i] == temp[i + 1]) {
			n++;
			i++;
		}
		#endif
		if(n > 0) {
			n++;
			score += n * (n - 1) / 2;
		}
	}
	return score;
}

// wide スケッチ用は，まずは，QBP のみで 
// optimize_pivot_by_precision // LS and AIR

