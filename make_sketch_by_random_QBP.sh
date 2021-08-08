#!/bin/bash
if [ $# -ne 4 ] ; then
echo "Usage>"
echo "1.  width          = sketch width"
echo "2.  sample_dataset = dataset number (00, ... , 96) of sample data" 
echo "3.  sample_size    = number of data in sample to computer radius"
echo "4.  seed           = random seed"
exit 1
fi

source ./set_directory.sh INTEL
pr_dir=/mnt/d/DISA_h/expanded

wd=$1; shift
sd=$1; shift
ss=$1; shift
seed=$1; shift

pivot_file="${pv_dir}/pivot_QBP_random_w${wd}_sd${sd}_ss${ss}.csv"
sample_dataset="${ds_dir}/fc6_${sd}.ftr"

cflags0="-O3 -Wall -Wno-strict-overflow"
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
cflags0="$cflags0 -DPARTITION_TYPE_QBP"
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

pr=make_sketch_by_random_QBP

gcc $cflags0 $cflags $pr_dir/$pr.c $lb -o $pr

if [ $? == 1 ] ; then
exit 1
fi

time ./$pr $ds

