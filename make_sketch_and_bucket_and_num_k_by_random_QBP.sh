#!/bin/bash
if [ $# -ne 8 ] ; then
echo "Usage>"
echo "1.  width          = sketch width"
echo "2.  sample_dataset = dataset number (00, ... , 96) of sample data" 
echo "3.  sample_size    = number of data in sample to computer radius"
echo "4.  seed           = random seed"
echo "5.  range          = 00_01, 00_09, or 00_96"
echo "6.  priority       = 0 -> Hamming, 1 -> score_1, 2 -> score_2, 3 -> score_inf"
echo "7.  query_file     = query file number, (00 or 90)"
echo "8.  #threads       = number of threads"
exit 1
fi

ulimit -Sn 4000

source ./set_directory.sh INTEL

wd=$1; shift
sd=$1; shift
ss=$1; shift
seed=$1; shift
range=$1; shift
priority=$1; shift
query=$1; shift
nt="$1"; shift

if [ $nt == 1 ] ; then
cflags2="-O3 -Wall -Wno-strict-overflow"
else
cflags2="-O3 -fopenmp -Wall -Wno-strict-overflow"
cflags2="$cflags2 -DNUM_THREADS=$nt"
fi

if [ $priority -ne 0 ] && [ $priority -ne 1 ] && [ $priority -ne 2 ] && [ $priority -ne 3 ] ; then
echo "invalid priority: 0, 1, 2, or 3 is required"
exit
fi

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

pv="pivot_QBP_random_w${wd}_sd${sd}_ss${ss}"
#pivot="${pv_dir}/pivot_QBP_random_w${wd}_sd${sd}_ss${ss}"
pivot_file="${pv_dir}/${pv}.csv"
sample_dataset="${ds_dir}/fc6_${sd}.ftr"
bk="$bk_dir/${pv}_${range}.bkt"

s=""
if [ $range == "00_00" ] ; then
  g=$ds_dir/fc6_00.ftr
  if [ ! -e $g ]; then
    echo dataset file = $g does not exist
    exit
  fi
  s="$s $g"
elif [ $range == "00_01" ] ; then
for f in {00..01} ; do
  g=$ds_dir/fc6_$f.ftr
  if [ ! -e $g ]; then
    echo dataset file = $g does not exist
    exit
  fi
  s="$s $g"
done
elif [ $range == "00_09" ] ; then
for f in {00..09} ; do
  g=$ds_dir/fc6_$f.ftr
  if [ ! -e $g ]; then
    echo dataset file = $g does not exist
    exit
  fi
  s="$s $g"
done
elif [ $range == "00_96" ] ; then
for f in {00..96} ; do
  g=$ds_dir/fc6_$f.ftr
  if [ ! -e $g ]; then
    echo dataset file = $g does not exist
    exit
  fi
  s="$s $g"
done
fi

cflags1="-O3 -Wall -Wno-strict-overflow"
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
cflags0="$cflags0 -DSAMPLE_SIZE=$ss"
cflags0="$cflags0 -DPARTITION_TYPE_QBP"
cflags0="$cflags0 -DFTR_ON_SECONDARY_MEMORY"
cflags0="$cflags0 -DFTR_ARRANGEMENT_ASIS"
cflags0="$cflags0 -DSEQUENTIAL_FILTERING"

cflags="$cflags -DPIVOT_FILE=\"${pivot_file}\""
cflags="$cflags -DSAMPLE_DATASET=\"${sample_dataset}\""
cflags="$cflags -DBUCKET_FILE=\"$bk\""
cflags="$cflags -DQUERY_FILE=\"$qr\""
cflags="$cflags -DANSWER_FILE=\"$an\""

echo "cflags1 = " $cflags1
echo "cflags2 = " $cflags2
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
gcc $cflags1 $cflags0 -c $lp

if [ $? == 1 ] ; then
echo compile error for libraly
exit 1
fi

lb="$pr_dir/$l1.o $pr_dir/$l2.o $pr_dir/$l3.o $pr_dir/$l4.o $pr_dir/$l5.o $pr_dir/$l6.o -lm"

pr=make_sketch_by_random_QBP

gcc $cflags1 $cflags0 $cflags $pr_dir/$pr.c $lb -o $pr

if [ $? == 1 ] ; then
exit 1
fi

time ./$pr $ds

pr=make_bucket

set -x

gcc $cflags2 $cflags0 $cflags $pr_dir/$pr.c $lb -o $pr

if [ $? == 1 ] ; then
exit 1
fi

time ./$pr $s

pr=num_k_for_recall_by_SF

gcc $cflags2 $cflags0 $cflags $pr_dir/$pr.c $lb -lm -o $pr

if [ $? == 1 ] ; then
exit 1
fi

time ./$pr


