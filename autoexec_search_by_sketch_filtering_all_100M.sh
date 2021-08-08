#!/bin/bash

set -x

ulimit -Sn 4000

s=INTEL
r=00_96
t=1
k=0
q=100
# m = 0 -> sequential filtering, 1 -> using bucket, 2 -> filtering by sketch enumeration, 3 -> filtering by sketch enumeration c2_n"
m=0
# f = 0 -> main memory, 1 -> secondary memory, 2 -> secondary memory (using sorted ftr), 3 -> secondary memory (using unsorted catenated ftr), 4-> main memory (sorted)"
f=1

t=32

./autoexec_search_by_sketch_filtering.sh 512 $r 1 2 5 10 0 0 $k $q $t $m $f $s

#for t in 16 ; do
#./autoexec_search_by_sketch_filtering.sh 14 $r 483 0 203 285 483 1137 $k $q $t $m $f $s
#./autoexec_search_by_sketch_filtering.sh 16 $r 367 0 161 250 367 604  $k $q $t $m $f $s
#./autoexec_search_by_sketch_filtering.sh 18 $r 254 0 113 169 254 558  $k $q $t $m $f $s
#./autoexec_search_by_sketch_filtering.sh 20 $r 231 0  95 146 231 408  $k $q $t $m $f $s
#./autoexec_search_by_sketch_filtering.sh 22 $r 178 0  74 100 178 304  $k $q $t $m $f $s
#./autoexec_search_by_sketch_filtering.sh 24 $r 137 0  57  82 137 292  $k $q $t $m $f $s
#./autoexec_search_by_sketch_filtering.sh 26 $r 123 0  46  70 123 265  $k $q $t $m $f $s
#./autoexec_search_by_sketch_filtering.sh 28 $r  90 0  37  55  90 221  $k $q $t $m $f $s
#./autoexec_search_by_sketch_filtering.sh 30 $r  83 0  33  50  83 197  $k $q $t $m $f $s
#done
#done
#m=3
#t=1
#./autoexec_search_by_sketch_filtering.sh 14 $r 483 0 203 285 483 1137 $k $q $t $m $f $s
#./autoexec_search_by_sketch_filtering.sh 16 $r 367 0 161 250 367 604  $k $q $t $m $f $s
#./autoexec_search_by_sketch_filtering.sh 18 $r 254 0 113 169 254 558  $k $q $t $m $f $s
#./autoexec_search_by_sketch_filtering.sh 20 $r 231 0  95 146 231 408  $k $q $t $m $f $s
#./autoexec_search_by_sketch_filtering.sh 22 $r 178 0  74 100 178 304  $k $q $t $m $f $s
#./autoexec_search_by_sketch_filtering.sh 24 $r 137 0  57  82 137 292  $k $q $t $m $f $s
#./autoexec_search_by_sketch_filtering.sh 26 $r 123 0  46  70 123 265  $k $q $t $m $f $s
#./autoexec_search_by_sketch_filtering.sh 28 $r  90 0  37  55  90 221  $k $q $t $m $f $s
#./autoexec_search_by_sketch_filtering.sh 30 $r  83 0  33  50  83 197  $k $q $t $m $f $s

# 100
#./autoexec_search_by_sketch_filtering.sh 14 $r 113 154 203 285 483 1137 $k $q $t $m $f $s
#./autoexec_search_by_sketch_filtering.sh 16 $r  85 116 161 250 367 604  $k $q $t $m $f $s
#./autoexec_search_by_sketch_filtering.sh 18 $r  63  85 113 169 254 558  $k $q $t $m $f $s
#./autoexec_search_by_sketch_filtering.sh 20 $r  49  69  95 146 231 408  $k $q $t $m $f $s
#./autoexec_search_by_sketch_filtering.sh 22 $r  40  55  74 100 178 304  $k $q $t $m $f $s
#./autoexec_search_by_sketch_filtering.sh 24 $r  25  37  57  82 137 292  $k $q $t $m $f $s
#./autoexec_search_by_sketch_filtering.sh 26 $r  20  29  46  70 123 265  $k $q $t $m $f $s
#./autoexec_search_by_sketch_filtering.sh 28 $r  20  25  37  55  90 221  $k $q $t $m $f $s
#./autoexec_search_by_sketch_filtering.sh 30 $r  17  23  33  50  83 197  $k $q $t $m $f $s
