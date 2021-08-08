#!/bin/bash
if [ $# -ne 3 ] ; then
echo "Usage> スクリプト.sh pivot bucket query.ftr answer.csv result.csv num_can num_k #threads dataset_1.ftr ... dataset_n.ftr"
echo "1. width       = sketch width" 
echo "2. range       = range of files, (ex. 00_09)" 
echo "3. #threads    = number of threads"
exit 1
fi

source ./set_directory.sh

w=$1; shift
pivot=$(grep w$w ./pivot.txt)

pv="$pv_dir/${pivot}.csv"
if [ ! -e $pv ]; then
  echo pivot file = $pv does not exist.
  exit
fi

range=$1; shift

bk="$bk_dir/${pivot}_${range}.bkt"
if [ ! -e $bk ]; then
  echo bucket file = $bk does not exist.
  exit
fi
wd=$(./pivot_property.sh -w $pv)
dm=$(./pivot_property.sh -d $pv)

qr="$qr_dir/fc6_q_00.ftr"
if [ ! -e $qr ]; then
  echo query file = $qr does not exist.
  exit
fi

an="$qr_dir/fc6_q_00_${range}.csv"
if [ ! -e $an ]; then
  echo answer file = $an does not exist.
  exit
fi

nt="$1"; shift

#if [ $nt == 1 ] ; then
#cflags0="-O3 -Wall -Wno-strict-overflow"
#else
cflags0="-O3 -fopenmp -Wall -Wno-strict-overflow"
cflags0="$cflags0 -DNUM_THREADS=$nt"
#fi

cflags0="$cflags0 -DFTR_DIM=$dm"
cflags0="$cflags0 -DPJT_DIM=$wd"
if [ $wd -gt 64 ] ; then
cflags0="$cflags0 -DEXPANDED_SKETCH"
elif [ $wd -gt 32 ] ; then
cflags0="$cflags0 -DWIDE_SKETCH"
else
cflags0="$cflags0 -DNARROW_SKETCH"
fi
cflags0="$cflags0 -DFTR_ON_MAIN_MEMORY"
cflags0="$cflags0 -DFTR_ARRANGEMENT_ASIS"
cflags0="$cflags0 -DSEQUENTIAL_FILTERING"
cflags0="$cflags0 -DPARTITION_TYPE_QBP"

#cflags="$cflags -DFTR_ON=MAIN_MEMORY"
#cflags="$cflags -DFTR_ARRANGEMENT=SORT_BY_SKETCH_AFTER_LOADING"
#cflags="$cflags -DFILTERING_BY=SKETCH_ENUMERATION"

cflags="$cflags -DPIVOT_FILE=\"$pv\""
cflags="$cflags -DBUCKET_FILE=\"$bk\""
cflags="$cflags -DQUERY_FILE=\"$qr\""
cflags="$cflags -DANSWER_FILE=\"$an\""
#cflags="$cflags -DRESULT_FILE=\"$rs\""
#cflags="$cflags -DNUM_CANDIDATES=$nc"
#cflags="$cflags -DNUM_K=$nk"

echo $cflags0
exit 0
echo $cflags

l1=bit_op
l2=ftr
l3=kNN_search
l4=sketch
l5=quick

lp="$pr_dir/$l1.c $pr_dir/$l2.c $pr_dir/$l3.c $pr_dir/$l4.c $pr_dir/$l5.c"

gcc $cflags0 -c $lp

if [ $? == 1 ] ; then
echo compile error for libraly
exit 1
fi

lb="$pr_dir/$l1.o $pr_dir/$l2.o $pr_dir/$l3.o $pr_dir/$l4.o $pr_dir/$l5.o"

pr=num_k_for_recall

gcc $cflags0 $cflags $pr_dir/$pr.c $lb -o $pr

if [ $? == 1 ] ; then
exit 1
fi

time ./$pr

