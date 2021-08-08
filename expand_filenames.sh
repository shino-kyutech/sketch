#!/bin/bash
if [ $# -ne 3 ] ; then
echo "Usage> expand_filenames fc6_ .ftr"
echo "1. prefix      = prefix"
echo "2. range       = 00_01, 00_09, or 00_96"
echo "3. sufix       = sufix"
exit 1
fi

prefix=$1; shift
range=$1; shift
sufix=$1; shift

if [ ${range} == "00" ] ; then
s=${prefix}${range}${sufix}
elif [ ${range} == "00_01" ] ; then
s=""
for f in {00..01} ; do
s="$s ${prefix}$f${sufix}"
done
elif [ ${range} == "00_09" ] ; then
s=""
for f in {00..09} ; do
s="$s ${prefix}$f${sufix}"
done
elif [ ${range} == "00_96" ] ; then
s=""
for f in {00..96} ; do
s="$s ${prefix}$f${sufix}"
done
fi

echo $s
