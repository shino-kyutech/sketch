#!/bin/bash

set -x

ulimit -Sn 4000

s=INTEL
r=00_09
t=1
k=10
q=1000
# m = 0 -> sequential filtering, 1 -> using bucket, 2 -> filtering by sketch enumeration, 3 -> filtering by sketch enumeration c2_n, 4 -> filtering by hamming"
m=0
# f = 0 -> main memory, 1 -> secondary memory, 2 -> secondary memory (using sorted ftr), 3 -> secondary memory (using unsorted catenated ftr), 4-> main memory (sorted)"
f=0

for m in 4 ; do
for t in 16 ; do
#for t in 24 28 32 ; do

./autoexec_search_by_sketch_filtering.sh 18 $r 149 210 307 0 100 120 $k $q $t $m $f $s

#./autoexec_search_by_sketch_filtering.sh 16 $r 369 169 0 256 369 660 $k $q $t $m $f $s
#./autoexec_search_by_sketch_filtering.sh 18 $r 20 40 60 80 100 200 $k $q $t $m $f $s
#./autoexec_search_by_sketch_filtering.sh 18 $r 262 117 0 167 262 555 $k $q $t $m $f $s
#./autoexec_search_by_sketch_filtering.sh 20 $r 242  98 0 151 242 420 $k $q $t $m $f $s
#./autoexec_search_by_sketch_filtering.sh 22 $r 178  78 0 103 178 320 $k $q $t $m $f
#./autoexec_search_by_sketch_filtering.sh 24 $r 140  58 0  80 140 303 $k $q $t $m $f $s
#./autoexec_search_by_sketch_filtering.sh 26 $r 20 40 60 80 100 125 $k $q $t $m $f $s
#./autoexec_search_by_sketch_filtering.sh 26 $r 125  47 0  72 125 267 $k $q $t $m $f $s
#./autoexec_search_by_sketch_filtering.sh 28 $r  99  39 0  58  99 219 $k $q $t $m $f
#./autoexec_search_by_sketch_filtering.sh 30 $r  90  35 0  54  90 196 $k $q $t $m $f
done
done

# 100
#./autoexec_search_by_sketch_filtering.sh 16 $r 88 122 169 256 369 660 $k $t $m $f
#./autoexec_search_by_sketch_filtering.sh 18 $r 64  87 117 167 262 555 $k $t $m $f
#./autoexec_search_by_sketch_filtering.sh 20 $r 49  69  98 151 242 420 $k $t $m $f
#./autoexec_search_by_sketch_filtering.sh 22 $r 40  59  78 103 178 320 $k $t $m $f
#./autoexec_search_by_sketch_filtering.sh 24 $r 26  38  58  80 140 303 $k $t $m $f
#./autoexec_search_by_sketch_filtering.sh 26 $r 21  29  47  72 125 267 $k $t $m $f
#./autoexec_search_by_sketch_filtering.sh 28 $r 20  26  39  58  99 219 $k $t $m $f
#./autoexec_search_by_sketch_filtering.sh 30 $r 17  24  35  54  90 196 $k $t $m $f
