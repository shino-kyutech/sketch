// バケットファイルの情報を用いて，特徴データがスケッチ順に並んだファイル（sftr）を作成する．
// 入力ファイルからは順に（sequential）読み込み，スケッチ順に並べ替えた主記憶上の配列に格納してソートする．
// 引数1: ftrファイル1
// 引数2: ftrファイル2
// ...
// 以下のものはマクロで定義して与える
// ピボットファイル（csv形式），スケッチ幅（w），スケッチファイル（バイナリファイル）

// read_compact_bucketでbktファイルを読み込む．スケッチ順の情報は idx にある．バケット表 bkt の展開は不要．
// 入力は，bktを作成したファイル群（1Mに分割した ftr ファイル列）
// ファイル群の通しデータ番号から，対応するファイル番号とデータ番号を求める．
// idx[i] = スケッチ順に並べたときに i 番目のデータの元の特徴データファイルでのデータ番号

#include "config.h"
#include "bit_op.h"
#include "ftr.h"
// #include "kNN_search.h"
#include "sketch.h"

int main(int argc, char *argv[])
{
	int num_ftr_files = argc - 1;
	char **dataset_ftr_filename = argv + 1;
	struct_dataset *ds;
	int num_data = 0;
	char *bucket_filename = BUCKET_FILE;
	char *sftr_filename = SFTR_FILE;

	// バケットをコンパクトに（bktを展開しないで）読み込む
	struct_bucket *bucket_ds = read_compact_bucket(bucket_filename);
	fprintf(stderr, "read compact bucket OK, ");
	num_data = bucket_ds->num_data;
	fprintf(stderr, "number of data = %d\n", num_data);

	// すべての ftr ファイルをいったん配列に読み込む
	ds = read_dataset_n(num_ftr_files, dataset_ftr_filename);
	fprintf(stderr, "read dataset file(s) OK. total number of data = %d\n", ds->num_data);
	if(num_data != ds->num_data) {
		fprintf(stderr, "bucket (filename = %s) is not compatible with datasets (filename = %s, ... )\n", bucket_filename, dataset_ftr_filename[0]);
		return -1;
	}

	// bkt の idx に基づいて，ソートする．
	sort_dataset(ds, bucket_ds->idx);

	FILE *fp;
	if((fp = fopen(sftr_filename, "wb"))  == NULL) {
		fprintf(stderr, "cannot write open sftr file, file name = %s\n", sftr_filename);
		return -1;
	}

	ftr_header_type hd = {sizeof(ftr_element_type), FTR_DIM, num_data, sizeof(auxi_type)};
	if(fwrite(&hd, sizeof(ftr_header_type), 1, fp) != 1) {  // ヘッダを書き出す
		fprintf(stderr, "fwrite error (ftr_header) file = %s\n", sftr_filename);
		return -1;
	}

	int i;
	for(i = 0; i < num_data; i++) {
		if(i % 10000 == 0) fprintf(stderr, "*");
		if(fwrite(&ds->ftr_id[i], sizeof(struct_ftr_id), 1, fp) != 1) { // スケッチ順で i 番目のレコード（ftrとid）を書き出す
			fprintf(stderr, "fwrite error (ftr_id), i = %d, idx[%d] = %d\n", i, i, bucket_ds->idx[i]);
			return -1;
		}
	}
	fclose(fp);
	free_bucket(bucket_ds);
	
	return 0;
}
