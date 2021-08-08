#!/bin/bash
if [ $# -ne 8 ] ; then
echo "Usage>"
echo "1.  width          = sketch width"
echo "2.  num_part       = number of partitions of PQBP"
echo "3.  num_trial_QBP  = number of trials for each pivot selection in QBP"
echo "4.  sample_dataset = dataset number (00, ... , 96) of sample data" 
echo "5.  sample_size    = number of data in sample"
echo "6.  ss_rad         = number of sample data for radius calculation"
echo "7.  #threads       = number of threads"
echo "8.  seed           = random seed"
exit 1
fi

ulimit -Sn 4000

source ./set_directory.sh INTEL
#pr_dir=/mnt/d/DISA_h/expanded

wd=$1; shift
np=$1; shift
tr=$1; shift
sd=$1; shift
ss=$1; shift
sr=$1; shift
nt=$1; shift
seed=$1; shift

pivot_file="${pv_dir}/pivot_PQBP_t${tr}_w${wd}_sd${sd}_ss${ss}_np${np}_sr${sr}_seed${seed}.csv"
sample_dataset="${ds_dir}/fc6_${sd}.ftr"

cflags0="-O3 -fopenmp -Wall -Wno-strict-overflow"
cflags0="$cflags0 -DNUM_THREADS=$nt"
cflags0="$cflags0 -DSEED=$seed"
cflags0="$cflags0 -DFTR_DIM=4096"
cflags0="$cflags0 -DPJT_DIM=$wd"
if [ $wd -gt 64 ] ; then
cflags0="$cflags0 -DEXPANDED_SKETCH"
elif [ $wd -gt 32 ] ; then
cflags0="$cflags0 -DWIDE_SKETCH"
else
cflags0="$cflags0 -DNARROW_SKETCH"
fi
cflags0="$cflags0 -DSAMPLE_SIZE=$ss"
cflags0="$cflags0 -DSAMPLE_SIZE_FOR_RADIUS=$sr"
cflags0="$cflags0 -DNUM_TRIAL_QBP=$tr"
#cflags0="$cflags0 -DPARTITION_TYPE_QBP"
cflags0="$cflags0 -DPARTITION_TYPE_PQBP"
cflags0="$cflags0 -DNUM_PART=$np"
cflags0="$cflags0 -DFTR_ON_MAIN_MEMORY"
cflags0="$cflags0 -DFTR_ARRANGEMENT_ASIS"
cflags0="$cflags0 -DSEQUENTIAL_FILTERING"

cflags="$cflags -DPIVOT_FILE=\"${pivot_file}\""
cflags="$cflags -DSAMPLE_DATASET=\"${sample_dataset}\""

echo $cflags0
echo $cflags


l1=bit_op
l2=ftr
l3=kNN_search
l4=sketch
l5=quick
l6=pivot_selection

lp="$pr_dir/$l1.c $pr_dir/$l2.c $pr_dir/$l3.c $pr_dir/$l4.c $pr_dir/$l5.c $pr_dir/$l6.c"

set "-x"

#gcc $cflags0 $cflags -c $lp
gcc $cflags0 -c $lp

if [ $? == 1 ] ; then
echo compile error for libraly
exit 1
fi

lb="$pr_dir/$l1.o $pr_dir/$l2.o $pr_dir/$l3.o $pr_dir/$l4.o $pr_dir/$l5.o $pr_dir/$l6.o -lm"

pr=make_sketch_by_QBP

gcc $cflags0 $cflags $pr_dir/$pr.c $lb -o $pr

if [ $? == 1 ] ; then
exit 1
fi

time ./$pr $ds

