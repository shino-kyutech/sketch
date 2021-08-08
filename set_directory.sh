#!/bin/bash

# $1 = HDD or SATA or SSD for dataset and sftr

if [ $1 == HDD ] ; then
echo "FTR ON HDD"
ds_dir="/mnt/d/DISA_h/dataset"			# データセット
sf_dir="/mnt/d/DISA_h/sftr"				# sftr データセット
elif [ $1 == SATA ] ; then
echo "FTR ON SATA"
ds_dir="/mnt/e/DISA_h/dataset"			# データセット
sf_dir="/mnt/e/DISA_h/sftr"				# sftr データセット
elif [ $1 == ADATA ] ; then
echo "FTR ON SSD-SATA"
ds_dir="/mnt/f/DISA_h/dataset"			# データセット
sf_dir="/mnt/g/DISA_h/sftr"				# sftr データセット
elif [ $1 == INTEL ] ; then
echo "FTR ON SSD"
ds_dir="/mnt/f/DISA_h/dataset"			# データセット
sf_dir="/mnt/f/DISA_h/sftr"				# sftr データセット
fi

qr_dir="/mnt/f/DISA_h/query"			# 質問 (ftr) と正解 (csv)
pv_dir="./pivot"		# ピボット (csv)
bk_dir="/mnt/e/DISA_h/bkt"			# バケット
pr_dir="."				# ソースプログラム

