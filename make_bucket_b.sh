#!/bin/bash
if [ $# -ne 3 ] ; then
echo "Usage> "
echo "1. pivot       = pivot file (wihtout .csv)"
echo "2. range       = 00_01, 00_09, or 00_96"
echo "3. #threads    = number of threads"
exit 1
fi

ulimit -Sn 4000

source ./set_directory.sh INTEL

pivot=$1; shift

range=$1; shift
nt="$1"; shift

pv=${pv_dir}/${pivot}.csv
if [ ! -e $pv ]; then
  echo pivot file = $pv does not exist.
  exit
fi
wd=$(./pivot_property.sh -w $pv)
dm=$(./pivot_property.sh -d $pv)
pt=$(./pivot_property.sh -p $pv)
np=$(./pivot_property.sh -n $pv)

bk="$bk_dir/${pivot}_${range}.bkt"

if [ ! -d ${bk_dir} ]; then
  echo バケットファイルを書き出すディレクトリ ${bk_dir} が存在しません．作成します．
  # 存在しない場合は作成
  mkdir ${bk_dir}
  if [ ! -d ${bk_dir} ]; then
    echo 作成できませんでした．
    exit;
  fi
fi

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
cflags0="$cflags0 -DFTR_ON_SECONDARY_MEMORY"
cflags0="$cflags0 -DFTR_ARRANGEMENT_ASIS"
cflags0="$cflags0 -DSEQUENTIAL_FILTERING"

if [ $pt == 3 ] ; then
cflags0="$cflags0 -DPARTITION_TYPE_PQBP"
cflags0="$cflags0 -DNUM_PART=$np"
else
cflags0="$cflags0 -DPARTITION_TYPE_QBP"
fi

cflags0="$cflags0 -DBLOCK_SIZE=200"
cflags="$cflags -DPIVOT_FILE=\"$pv\""
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

gcc $cflags0 -c $lp

if [ $? == 1 ] ; then
echo compile error for libraly
exit 1
fi

lb="$pr_dir/$l1.o $pr_dir/$l2.o $pr_dir/$l3.o $pr_dir/$l4.o $pr_dir/$l5.o -lm"

pr=make_bucket

set -x

gcc $cflags0 $cflags $pr_dir/$pr.c $lb -o $pr

if [ $? == 1 ] ; then
exit 1
fi

time ./$pr $s

