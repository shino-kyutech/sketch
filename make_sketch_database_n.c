// 特徴データのスケッチを作成してファイルに保存する．検索はしない．
// 引数1: ftrファイル1
// 引数2: ftrファイル2
// ...
// 以下のものはマクロで定義して与える
// ピボットファイル（csv形式），スケッチ幅（w），スケッチファイル（バイナリファイル）

#include "config.h"
#include "bit_op.h"
#include "ftr.h"
#include "kNN_search.h"
#include "sketch.h"

int main(int argc, char *argv[])
{
	int num_ftr_files = argc - 1;
	char **dataset_ftr_filename = argv + 1;
	file_handle fh[num_ftr_files];
	ftr_header_type header[num_ftr_files];
	char *pivot_file = PIVOT_FILE;
	char *sketch_file = SKETCH_FILE;

	// まず，すべての ftr ファイルをオープンしてデータ総数を調べておく
	int num_data = 0;
	int offset[num_ftr_files];
	for(int f = 0; f < num_ftr_files; f++) {
		offset[f] = num_data;
		if((fh[f] = open_ftr_file(dataset_ftr_filename[f], &header[f])) == OPEN_ERROR) {
			fprintf(stderr, "cannor open ftr file %s\n", dataset_ftr_filename[f]);
			return -1;
		}
		fprintf(stderr, "number of data in %s = %d\n", dataset_ftr_filename[f], header[f].num_data);
		num_data += header[f].num_data;
	}
	fprintf(stderr, "open dataset file(s) OK. total number of data = %d\n", num_data);
	
	fprintf(stderr, "PJT_DIM=%d\n", PJT_DIM);
	pivot_type *pivot = new_pivot(QBP);
	read_pivot(pivot_file, pivot);
	fprintf(stderr, "read pivot OK\n");

	fprintf(stderr, "make sketch of data ... \n");
	sketch_type *sketch = (sketch_type *)malloc(sizeof(sketch_type) * num_data);
	struct_ftr_id ftr_id[num_ftr_files];
	#ifdef _OPENMP
	#pragma omp parallel for
	#endif
	for(int f = 0; f < num_ftr_files; f++) {
		for(int i = 0; i < header[f].num_data; i++) {
			if(!get_ftr_id(fh[f], &header[f], i, &ftr_id[f])) {
				fprintf(stderr, "cannot get ftr: data_num = %d in file %s.\n", i, dataset_ftr_filename[f]);
				exit(-1);
			}
			sketch[offset[f] + i] = data_to_sketch(ftr_id[f].ftr, pivot);
		}
	}
	fprintf(stderr, "make sketch DONE.\n");

	FILE *fp_sketch;
	fprintf(stderr, "open sketch file (%s) ... ", sketch_file);
	if((fp_sketch = fopen(sketch_file, "wb")) == NULL) {
		fprintf(stderr, "error\n");
		return -3;
	}
	fprintf(stderr, "OK\n");

	fprintf(stderr, "write sketches (file = %s) ... ", sketch_file);
	if(fwrite(sketch, sizeof(sketch_type) * num_data, 1, fp_sketch) != 1) {
		fprintf(stderr, "fwrite error\n");
		return -4;
	}
	fclose(fp_sketch);
	fprintf(stderr, "OK (data_num = %d)\n", num_data);

	return 0;
}
