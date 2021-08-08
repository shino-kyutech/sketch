#!/bin/bash

# RAM = 10GB (6GB)

p1=pivot_AIR1000000_w24_s302_k30_one_8th_pivot
p2=pivot_PQBP_t2_w256_sd00_ss100000_np128_sr10000_seed26
range=00_96
query=fc6_q_50
result=double_filtering_w24_w256_q005090.csv
k1=30000
k2=10
nk=100
nq=1000
s1=14
s2=10
nt=8


for nt in 8 16 ; do
for q in 50 90 00 ; do
query=fc6_q_$q
./search_by_double_filtering_multi.sh INTEL $p1 $p2 00_96 $query $result 32000 32000 1000 14 14 1 $nk $nq $s1 10 $nt
done
done

for nt in 8 16 ; do
for q in 50 90 00 ; do
query=fc6_q_$q
./search_by_double_filtering_multi.sh INTEL $p1 $p2 00_96 $query $result 35000 35000 1000 35 35 1 $nk $nq $s1 8 $nt
done
done

for nt in 8 16 ; do
for q in 50 90 00 ; do
query=fc6_q_$q
./search_by_double_filtering_multi.sh INTEL $p1 $p2 00_96 $query $result 60000 60000 10000 120 120 10 $nk $nq $s1 8 $nt
done
done
