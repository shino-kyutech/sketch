#!/bin/bash

w=4096
p=256
s=11
range=00_96

#for s in {10..15} ; do
#for p in 128 64 ; do
#./make_sketch_by_PQBP.sh $w $p 2 00 10000 10000 32 $s
pv=pivot_PQBP_t2_w${w}_sd00_ss10000_np${p}_sr10000_seed${s}
#./make_bucket_b.sh $pv $range 32
#./balance_check.sh INTEL $pv $range 16
./num_k_for_recall_by_SF_b.sh $pv 0.4 0.7 0.1 $range 00 32 > score_p_${pv}_${range}_add2.csv
#done
#done

