#!/bin/bash

# RAM = 28GB

p1=pivot_AIR1000000_w24_s302_k30_one_8th_pivot
p2=pivot_PQBP_t2_w2048_sd00_ss10000_np512_sr1000_seed25
range=00_96
query=fc6_q_50
result=double_filtering_w24_w2048_q005090_4.csv
k1=30000
k2=10
nk=100
nq=1000
s1=10
s2=5
nt=16

#./search_by_double_filtering_multi.sh INTEL $p1 $p2 00_96 $query $result 35000 55000 2000 2 6 1 $nk $nq $s1 $s2 $nt

#for nt in 8 16 ; do
#for q in 00 50 90 ; do
#query=fc6_q_$q
#./search_by_double_filtering_multi.sh INTEL $p1 $p2 00_96 $query $result 9000 9000 1000 2 2 1 $nk $nq $s1 $s2 $nt
#done
#done

#for nt in 8 16 ; do
#for q in 00 50 90 ; do
#query=fc6_q_$q
#./search_by_double_filtering_multi.sh INTEL $p1 $p2 00_96 $query $result 19000 19000 1000 2 2 1 $nk $nq $s1 $s2 $nt
#done
#done

for nt in 8 16 ; do
for q in 00 50 90 ; do
query=fc6_q_$q
./search_by_double_filtering_multi.sh INTEL $p1 $p2 00_96 $query $result 40000 40000 1000 6 6 1 $nk $nq $s1 $s2 $nt
done
done
