// バケットファイルの情報を用いて，特徴データがスケッチ順に並んだファイル（sftr）を作成する．
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
	dataset_handle dh;
	int num_data = 0;
	char *bucket_filename = BUCKET_FILE;
	char *sftr_filename = SFTR_FILE;

	// バケットをコンパクトに（bktを展開しないで）読み込む
	struct_bucket *bucket_ds = read_compact_bucket(bucket_filename);
	fprintf(stderr, "read compact bucket OK, ");
	num_data = bucket_ds->num_data;
	fprintf(stderr, "number of data = %d\n", num_data);
	fprintf(stderr, "BLOCK_SIZE = %d\n", BLOCK_SIZE);

	// すべての ftr ファイルをひとまとめに扱うために open_multi_ftr でオープンしてデータ総数を調べておく
	dh.ftr_on = SECONDARY_MEMORY;
	dh.sorted = 0;
	dh.ds = NULL;
	dh.num_threads = 1;
	dh.mf = (struct_multi_ftr **)malloc(sizeof(struct_multi_ftr *) * dh.num_threads);
	dh.mf[0] = open_multi_ftr(num_ftr_files, dataset_ftr_filename, BLOCK_SIZE);
	if(num_data != dh.mf[0]->num_data) {
		fprintf(stderr, "bucket (filename = %s) is not compatible with datasets (filename = %s, ... )\n", bucket_filename, dataset_ftr_filename[0]);
		return -1;
	}

	FILE *fp;
	if((fp = fopen(sftr_filename, "wb"))  == NULL) {
		fprintf(stderr, "cannot write open sftr file, file name = %s\n", sftr_filename);
		return -1;
	}
	ftr_header_type hd;
	hd = dh.mf[0]->hd[0]; // ftrヘッダを入力ファイルの先頭のものをコピー
	hd.num_data = num_data; // データ数は，合計に書き換える

	if(fwrite(&hd, sizeof(ftr_header_type), 1, fp) != 1) {  // ヘッダを書き出す
		fprintf(stderr, "fwrite error (ftr_header) file = %s\n", sftr_filename);
		return -1;
	}

	int i, j, b;
//	struct_ftr_id *ftr_id;
	for(i = j = 0; i < num_data; i += b, j++) {
		if(j % 100 == 0) fprintf(stderr, "*");
//		ftr_id = get_next_ftr_id_from_multi_ftr(dh.mf[0], bucket_ds->idx, i, num_data);
//		if(fwrite(ftr_id, sizeof(struct_ftr_id), 1, fp) != 1) { // スケッチ順で i 番目のレコード（ftrとid）を書き出す
//			fprintf(stderr, "fwrite error (ftr_id), i = %d, idx[%d] = %d\n", i, i, bucket_ds->idx[i]);
//			return -1;
//		}
		b = get_ftr_block(dh.mf[0], bucket_ds->idx, i, num_data); // スケッチ順に i 番目のレコードから BLOCK_SIZE 個のレコードをバッファに読み込む
//		fprintf(stderr, "b = %d\n", b); getchar();
		if(fwrite(dh.mf[0]->ftr_id, sizeof(struct_ftr_id), b, fp) != b) { // バッファに読込んだ b 個のレコード（ftrとid）を書き出す
			fprintf(stderr, "fwrite error (ftr_id), i = %d, idx[%d] = %d\n", i, i, bucket_ds->idx[i]);
			return -1;
		}
	}
	fclose(fp);
	close_multi_ftr(dh.mf[0]);
	free_bucket(bucket_ds);
	
	return 0;
}
