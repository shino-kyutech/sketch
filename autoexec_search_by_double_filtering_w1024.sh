#!/bin/bash

# RAM = 20GB (process = 15GB)

p1=pivot_AIR1000000_w24_s302_k30_one_8th_pivot
p2=pivot_PQBP_t5_w1024_sd00_ss100000_np512_sr100000_seed2
range=00_96
query=fc6_q_50
result=double_filtering_w24_w1024_q005090_2.csv
k1=30000
k2=10
nk=100
nq=101
s1=14
s2=5
nt=8


for nt in 32 ; do
for q in 00 ; do
query=fc6_q_$q
./search_by_double_filtering_multi.sh INTEL $p1 $p2 00_96 $query $result 10000 60000 10000 2 2 1 $nk $nq $s1 $s2 $nt
done
done

exit

for nt in 8 16 ; do
for q in 00 50 90 ; do
query=fc6_q_$q
./search_by_double_filtering_multi.sh INTEL $p1 $p2 00_96 $query $result 25000 25000 1000 2 3 1 $nk $nq $s1 $s2 $nt
done
done

for nt in 8 16 ; do
for q in 00 50 90 ; do
query=fc6_q_$q
./search_by_double_filtering_multi.sh INTEL $p1 $p2 00_96 $query $result 50000 50000 1000 5 6 1 $nk $nq $s1 $s2 $nt
done
done
