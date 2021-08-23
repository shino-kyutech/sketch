// バケットファイルの情報を用いて，特徴データがスケッチ順に並んだファイル（sftr）をマージする．
// 入力ファイルからは順に（sequential）読み込み，スケッチ順に並べ替えた主記憶上の配列に格納してソートする．
// 引数1 		出力ファイル（sftr）
// 引数2, 引数3, 引数4, 引数5, ... : bucket_file1, bucket_file2, ... , sftr_file1, sftr_file2, ...
// 最初に出力バケットファイル，入力バケットファイルを並べ，その後に対応するSFTRファイルを与える．

// read_compact_bucketでbktファイルを読み込む．スケッチ順の情報は idx にある．バケット表 bkt の展開は不要．
// 入力は，bktを作成したファイル群（1Mに分割した ftr ファイル列）
// ファイル群の通しデータ番号から，対応するファイル番号とデータ番号を求める．
// idx[i] = スケッチ順に並べたときに i 番目のデータの元の特徴データファイルでのデータ番号

//	int data_num; // データ数
//	ftr[0], ... , ftr[data_num - 1]; // 特徴データ（オリジナル）
//	sk[0], ... , sk[data_num - 1]; // スケッチ
//	sftr[0], ... , sftr[data_num - 1]; // 特徴データをスケッチ順に並べたもの
//	idx[i] = "sftr[i] のオリジナルのインデックス" （sftr[i] = ftr[idx[i]]）
//			「スケッチ順で i 番目のデータがオリジナルで何番目であるかと表す」
//	bkt[s] = スケッチ順にデータを並べたときに，スケッチが s であるものの先頭の位置 (0 - data_num - 1)
//	bkt[s + 1] - bkt[s] = スケッチが s であるもののデータ数
//
//	スケッチが s であるデータ = sftr[j] (j = bkt[s], ... , bkt[s + 1] - 1) (sftr が使えるとき)
//	スケッチが s であるデータ = ftr[idx[j]] (j = bkt[s], ... , bkt[s + 1] - 1) 
//	スケッチが s であるデータのオリジナルインデックス = idx[j] (j = bkt[s], ... , bkt[s + 1] - 1)

#include "config.h"
#include "bit_op.h"
#include "ftr.h"
// #include "kNN_search.h"
#include "sketch.h"

int main(int argc, char *argv[])
{
	char *merged_sftr_filename = argv[1];
	int num_sftr_files = (argc - 2) / 2;
	char **bucket_filename = argv + 2;
	char **sftr_filename = bucket_filename + num_sftr_files;
	for(int i = 0; i < num_sftr_files; i++) {
		printf("bucket[%d] = %s, sftr[%d] = %s\n", i, bucket_filename[i], i, sftr_filename[i]);
	}

	// 入力bktとsftrをopen
	int num_data = 0, num_data_in_sftr = 0;
	struct_bucket_sk_num *bsk[num_sftr_files];
	file_handle fh[num_sftr_files];
	ftr_header_type hd[num_sftr_files];
	for(int i = 0; i < num_sftr_files; i++) {
		bsk[i] = open_bucket_sk_num(bucket_filename[i]);
		num_data += bsk[i]->num_data;
		fh[i] = open_ftr_file(sftr_filename[i], &hd[i]);
		num_data_in_sftr += hd[i].num_data;
	}
	if(num_data != num_data_in_sftr) {
		fprintf(stderr, "number of data (%d) in bucket files is different from number of data in sftr files (%d)\n", num_data, num_data_in_sftr);
		return -1;
	}
	fprintf(stderr, "open bucket OK, number of data = %d\n", num_data);

	FILE *fp;
	if((fp = fopen(merged_sftr_filename, "wb"))  == NULL) {
		fprintf(stderr, "cannot write open sftr file, file name = %s\n", merged_sftr_filename);
		return -1;
	}
	fprintf(stderr, "write open OK: file = %s\n", merged_sftr_filename);

	ftr_header_type merged_hd = {sizeof(ftr_element_type), FTR_DIM, num_data, sizeof(auxi_type)};
	if(fwrite(&merged_hd, sizeof(ftr_header_type), 1, fp) != 1) {  // ヘッダを書き出す
		fprintf(stderr, "fwrite error (ftr_header) file = %s\n", merged_sftr_filename);
		return -1;
	}
	fprintf(stderr, "write header of sftr OK: file = %s\n", merged_sftr_filename);

	struct_ftr_id ftr_id[BLOCK_SIZE];		// buffer for ftr data with data_id
	int read_in = 0;
	int i;
	for(i = 0; i < num_data; ) {
		int f, min_f;
		for(min_f = 0; min_f < num_sftr_files; min_f++) {
			if(bsk[min_f]->processed_buckets < bsk[min_f]->num_nonempty_buckets) break;
		}
		if(min_f >= num_sftr_files) {
			fprintf(stderr, "no more data: number of output data = %d, num_data = %d, min_f = %d, num_sftr_files = %d\n", i, num_data, min_f, num_sftr_files);
			return -1;
		}
		for(f = min_f + 1; f < num_sftr_files; f++) {
			if(bsk[f]->processed_buckets >= bsk[f]->num_nonempty_buckets) continue;
			if(comp_sketch(bsk[f]->sk_num.sk, bsk[min_f]->sk_num.sk) < 0) {
				min_f = f;
			}
		}
		for(int j = 0; j < bsk[min_f]->sk_num.num; j++) {
			if(i % 1000000 == 0) fprintf(stderr, "*");
			if(!READ(fh[min_f], &ftr_id[read_in], sizeof(struct_ftr_id))) {
				fprintf(stderr, "read error (ftr_id), file number = %d\n", min_f);
				return -1;
			}
			read_in++;
			if(read_in == BLOCK_SIZE) {
				if(fwrite(ftr_id, sizeof(struct_ftr_id), read_in, fp) != read_in) {
					fprintf(stderr, "fwrite error (ftr_id), i = %d\n", i);
					return -1;
				}
				read_in = 0;
			}
			i++;
		}
		read_next_bucket_sk_num(bsk[min_f]);
	}
	if(read_in != 0) {
		if(fwrite(ftr_id, sizeof(struct_ftr_id), read_in, fp) != read_in) {
			fprintf(stderr, "fwrite error (ftr_id), i = %d\n", i);
			return -1;
		}
		read_in = 0;
	}
	if(i < num_data) {
		fprintf(stderr, "too small number of data: num_data = %d, num_written_data = %d\n", num_data, i);
	}
	for(int i = 0; i < num_sftr_files; i++) {
		fclose(bsk[i]->fp);
		CLOSE(fh[i]);
	}
	fclose(fp);
	
	return 0;
}
