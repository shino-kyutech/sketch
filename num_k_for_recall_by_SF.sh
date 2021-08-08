#!/bin/bash
if [ $# -ne 5 ] ; then
echo "Usage> スクリプト.sh priority width range #threads"
echo "1. priority    = 0 -> Hamming, 1 -> score_1, 2 -> score_2, 3 -> score_inf"
echo "2. width       = sketch width"
echo "3. range       = range of files, (ex. 00_09)"
echo "4. query_file  = query file number, (00 or 90)"
echo "5. #threads    = number of threads"
exit 1
fi

source ./set_directory.sh INTEL

priority=$1; shift

if [ $priority -ne 0 ] && [ $priority -ne 1 ] && [ $priority -ne 2 ] && [ $priority -ne 3 ] ; then
echo "invalid priority: 0, 1, 2, or 3 is required"
exit
fi

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
pt=$(./pivot_property.sh -p $pv)

query=$1; shift
qr="$qr_dir/fc6_q_${query}.ftr"
if [ ! -e $qr ]; then
  echo query file = $qr does not exist.
  exit
fi

an="$qr_dir/fc6_q_${query}_${range}.csv"
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
if [ $priority == 1 ] ; then
cflags0="$cflags0 -DPRIORITY=1"
cflags0="$cflags0 -DSCORE_1"
elif [ $priority == 2 ] ; then
cflags0="$cflags0 -DPRIORITY=2"
cflags0="$cflags0 -DSCORE_2"
elif [ $priority == 3 ] ; then
cflags0="$cflags0 -DPRIORITY=3"
cflags0="$cflags0 -DSCORE_INF"
else
cflags0="$cflags0 -DPRIORITY=${priority}"
fi
cflags0="$cflags0 -DFTR_ON_MAIN_MEMORY"
cflags0="$cflags0 -DFTR_ARRANGEMENT_ASIS"
cflags0="$cflags0 -DSEQUENTIAL_FILTERING"

if [ $pt == 3 ] ; then
cflags0="$cflags0 -DPARTITION_TYPE_PQBP"
else
cflags0="$cflags0 -DPARTITION_TYPE_QBP"
fi

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

pr=num_k_for_recall_by_SF

gcc $cflags0 $cflags $pr_dir/$pr.c $lb -lm -o $pr

if [ $? == 1 ] ; then
exit 1
fi

time ./$pr

