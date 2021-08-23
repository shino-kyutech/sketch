#ifndef COMPILE_TIME // 実際のコンパイル時にはCOMPILE_TIMEをdefineして，以下の定義を無効にすること．
                     // VSCODEでプログラムを作成している時に，未定義定数でエラーが出るのを防ぐために，
                     // 実際のコンパイル時に事前に定義して用いる定数を定義しておく． 

#define DEEP1B
#define NUM_K 10    // k-NN で求める近傍数
#define QUERY "query_xx.ftr"    // 質問データのftrファイル
#define SAMPLE_DATASET "base10M_00.ftr"     // ピボット選択時のサンプルデータセット
#define PIVOT_FILE "pivot.csv"
#define BUCKET_FILE "pivot_range.bkt"
#define QUERY_FILE "query_xx.ftr"
#define ANSWER_FILE "query_xx.csv"
#define SFTR_FILE "base_yy_xx.ftr"
#define SEED 1 // random seed
#define NUM_TRIAL_QBP 100

#define PARTITION_TYPE_QBP
//#define PARTITION_TYPE_PQBP

#define _OPENMP
#define NUM_THREADS 8
#define NARROW_SKETCH
//#define WIDE_SKETCH
//#define EXPANDED_SKETCH

#define PJT_DIM 16
#define SAMPLE_SIZE 10000
#define FTR_ON_MAIN_MEMORY
//#define FTR_ON_SECONDARY_MEMORY

#define RESULT_FILE "result/${pivot}_${range}.csv"
#define NUM_CANDIDATES  100 // データセットに対する割合（パーミリアド（万分率））: 100 -> 1%
#define NUM_CANDIDATES1 200 // データセットに対する割合（パーミリアド（万分率））: 200 -> 2%
#define NUM_CANDIDATES2 300
#define NUM_CANDIDATES3 400
#define NUM_CANDIDATES4 500
#define NUM_CANDIDATES5 600

//#define FILTERING_BY_SKETCH_ENUMERATION
#define FILTERING_BY_SKETCH_ENUMERATION_C2N
//#define SEQUENTIAL_FILTERING
//#define SEQUENTIAL_FILTERING_USING_BUCKET
//#define SEQUENTIAL_FILTERING_USING_HAMMING
#define DOUBLE_FILTERING

#define NUM_Q 100
#define NUM_D 10000
#define ACCESS_MODE "SEQUENTIAL"

#define EXPANDED_PIVOT_FILE     "pivot_PQBP_t10_w192_sd00_ss10000_np6_sr1000_seed1.csv"
#define EXPANDED_BUCKET_FILE    "pivot_PQBP_t10_w192_sd00_ss10000_np6_sr1000_seed1_00_99.bkt"
#define NARROW_PIVOT_FILE       "pivot_QBP_t1000_w28_sd00_ss10000.csv"
#define NARROW_BUCKET_FILE      "pivot_QBP_t1000_w28_sd00_ss10000_00_99.bkt"

#define NUM_CANDIDATES_1ST      3000
#define NUM_CANDIDATES_1ST_END  8000
#define NUM_CANDIDATES_1ST_STEP 1000
#define NUM_CANDIDATES_2ND      1
#define NUM_CANDIDATES_2ND_END  5
#define NUM_CANDIDATES_2ND_STEP 1

#define SCORE_P_1ST 1.0
#define SCORE_P_2ND 1.0

#endif
// これより下には追加しないこと