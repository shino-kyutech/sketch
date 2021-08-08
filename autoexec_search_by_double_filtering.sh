#!/bin/bash

p1=pivot_AIR1000000_w24_s302_k30_one_8th_pivot
p2=pivot_PQBP_t2_w512_sd00_ss100000_np128_sr10000_seed25
range=00_96
query=fc6_q_00
result=double_filtering_w24_w512.csv
k1=25000
k2=10
nk=100
nq=1000
s1=14
s2=7
nt=8

#for k1 in 10000 20000 30000 40000 50000 60000 ; do
for k1 in 25000 ; do
for k2 in 20 ; do
for nt in 16 ; do

./search_by_double_filtering.sh INTEL $p1 $p2 $range $query $result $k1 $k2 $nk $nq $s1 $s2 $nt

done
done
done
