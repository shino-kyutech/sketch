#!/bin/bash

cflags="-O3 -fopenmp -Wall -Wno-strict-overflow"
cflags="$cflags -DFTR_DIM=4096"
cflags="$cflags -DPJT_DIM=40"
cflags="$cflags -DWIDE_SKETCH"
cflags="$cflags -DFTR_ON_MAIN_MEMORY"
cflags="$cflags -DFTR_ARRANGEMENT_ASIS"
cflags="$cflags -DSEQUENTIAL_FILTERING"
cflags="$cflags -DPARTITION_TYPE_QBP"
echo $cflags

pr_dir="."
l1=bit_op
l2=ftr
l3=kNN_search
l4=sketch
l5=quick

lp="$pr_dir/$l1.c $pr_dir/$l2.c $pr_dir/$l3.c $pr_dir/$l4.c $pr_dir/$l5.c"

gcc $cflags -c $lp

if [ $? == 1 ] ; then
echo compile error for libraly
exit 1
fi

lb="$pr_dir/$l1.o $pr_dir/$l2.o $pr_dir/$l3.o $pr_dir/$l4.o $pr_dir/$l5.o"

pr=median

set -x

gcc $cflags $pr_dir/$pr.c $lb -o $pr

if [ $? == 1 ] ; then
exit 1
fi

time ./$pr

