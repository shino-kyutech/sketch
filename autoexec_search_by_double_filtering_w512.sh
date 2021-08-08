#!/bin/bash

# RAM = 12GB (9GB)

p1=pivot_AIR1000000_w24_s302_k30_one_8th_pivot
p2=pivot_PQBP_t2_w512_sd00_ss100000_np128_sr10000_seed25
range=00_96
query=fc6_q_50
result=double_filtering_w24_w512_q005090.csv
k1=30000
k2=10
nk=100
nq=1000
s1=14
s2=10
nt=8


for nt in 16 8 ; do
for q in 90 00 50 ; do
query=fc6_q_$q
./search_by_double_filtering_multi.sh INTEL $p1 $p2 00_96 $query $result 10000 10000 1000 10 10 1 $nk $nq $s1 $s2 $nt
done
done

for nt in 16 8 ; do
for q in 90 00 50 ; do
query=fc6_q_$q
./search_by_double_filtering_multi.sh INTEL $p1 $p2 00_96 $query $result 26000 26000 1000 7 7 1 $nk $nq $s1 $s2 $nt
done
done

for nt in 16 8 ; do
for q in 90 00 50 ; do
query=fc6_q_$q
./search_by_double_filtering_multi.sh INTEL $p1 $p2 00_96 $query $result 60000 60000 1000 13 13 1 $nk $nq $s1 $s2 $nt
done
done
