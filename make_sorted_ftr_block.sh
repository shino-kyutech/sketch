#!/bin/bash
if [ $# -ne 3 ] ; then
echo "Usage>"
echo "1. width       = sketch width"
echo "2. range       = range of ftr files" 
echo "3. block_size  = number of data in block access"
exit 1
fi

#set -x

source ./set_directory.sh INTEL

w=$1; shift
pivot=$(grep w$w ./pivot.txt)
echo $pivot

pv="$pv_dir/${pivot}.csv"
if [ ! -e $pv ]; then
  echo pivot file = $pv does not exist.
  exit
fi
wd=$(./pivot_property.sh -w $pv)
dm=$(./pivot_property.sh -d $pv)

range=$1; shift
bk="$bk_dir/${pivot}_${range}.bkt"
if [ ! -e $bk ]; then
  echo bucket file = $bk does not exist.
  exit
fi

block=$1; shift

sf="$sf_dir/${pivot}_${range}.sftr"

#cflags0="-O3 -Wall -Wno-strict-overflow"
cflags0="-O3 -fopenmp -Wall -Wno-strict-overflow"
cflags0="$cflags0 -DNUM_THREADS=16"

cflags0="$cflags0 -DFTR_DIM=$dm"
cflags0="$cflags0 -DPJT_DIM=$wd"
if [ $wd -gt 32 ] ; then
cflags0="$cflags0 -DWIDE_SKETCH"
else
cflags0="$cflags0 -DNARROW_SKETCH"
fi
cflags0="$cflags0 -DPARTITION_TYPE_QBP"
cflags0="$cflags0 -DBLOCK_SIZE=${block}"
cflags="$cflags -DFTR_ON_SECONDARY_MEMORY"
cflags="$cflags -DFTR_ARRANGEMENT_ASIS"
#cflags="$cflags -DBLOCK_SIZE=1"
cflags="$cflags -DSEQUENTIAL_FILTERING"
cflags="$cflags -DBUCKET_FILE=\"$bk\""
cflags="$cflags -DSFTR_FILE=\"$sf\""

echo $cflags

ds=$(./expand_filenames.sh "${ds_dir}/fc6_" $range ".ftr")

echo $ds

l1=bit_op
l2=ftr
l3=kNN_search
l4=sketch
l5=quick

lp="$pr_dir/$l1.c $pr_dir/$l2.c $pr_dir/$l3.c $pr_dir/$l4.c $pr_dir/$l5.c"

gcc $cflags0 $cflags -c $lp

if [ $? == 1 ] ; then
echo compile error for libraly
exit 1
fi

lb="$pr_dir/$l1.o $pr_dir/$l2.o $pr_dir/$l3.o $pr_dir/$l4.o $pr_dir/$l5.o"

pr=make_sorted_ftr_block

gcc $cflags0 $cflags $pr_dir/$pr.c $lb -o $pr

if [ $? == 1 ] ; then
exit 1
fi

time ./$pr $ds

