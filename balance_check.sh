#!/bin/bash
if [ $# -ne 4 ] ; then
echo "Usage>"
echo "1.  storage    = HDD, SSD, ADATA, INTEL"
echo "2.  pivot      = pivot file"
echo "3.  range      = range of ftr files" 
echo "4.  #threads   = number of threads"
exit 1
fi

#set -x

st=$1; shift
if [ $st == HDD ] || [ $st == SSD ] || [ $st == ADATA ] || [ $st == INTEL ] ; then
source ./set_directory.sh $st
else
echo "invalid storage $st: storage = HDD, SSD, ADATA, or INTEL"
exit
fi

pivot=$1; shift

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

ft="SEQUENTIAL_FILTERING"
fo="SECONDARY_MEMORY"
fr="ARRANGEMENT_ASIS"

nt="$1"; shift

if [ $nt == 1 ] ; then
cflags0="-O3 -Wall -Wno-strict-overflow"
else
cflags0="-O3 -fopenmp -Wall -Wno-strict-overflow"
cflags0="$cflags0 -DNUM_THREADS=$nt"
fi

cflags0="$cflags0 -DFTR_DIM=$dm"
cflags0="$cflags0 -DPJT_DIM=$wd"
if [ $wd -gt 64 ] ; then
cflags0="$cflags0 -DEXPANDED_SKETCH"
elif [ $wd -gt 32 ] ; then
cflags0="$cflags0 -DWIDE_SKETCH"
else
cflags0="$cflags0 -DNARROW_SKETCH"
fi
cflags0="$cflags0 -DPRIORITY=1"
cflags0="$cflags0 -DSCORE_1"
cflags0="$cflags0 -DPARTITION_TYPE_QBP"
cflags0="$cflags0 -DFTR_ON_${fo}"
cflags0="$cflags0 -DFTR_${fr}"

cflags="$cflags -D${ft}"
cflags="$cflags -DBUCKET_FILE=\"$bk\""

echo $cflags0
echo $cflags


l1=bit_op
l2=ftr
l3=kNN_search
l4=sketch
l5=quick
#l6=bucket

lp="$pr_dir/$l1.c $pr_dir/$l2.c $pr_dir/$l3.c $pr_dir/$l4.c $pr_dir/$l5.c"

gcc $cflags0 $cflags -c $lp

if [ $? == 1 ] ; then
echo compile error for libraly
exit 1
fi

lb="$pr_dir/$l1.o $pr_dir/$l2.o $pr_dir/$l3.o $pr_dir/$l4.o $pr_dir/$l5.o -lm"

pr=balance_check

gcc $cflags0 $cflags $pr_dir/$pr.c $lb -o $pr

if [ $? == 1 ] ; then
exit 1
fi

time ./$pr

