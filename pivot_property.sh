#!/bin/bash

if [ $1 == "-w" ] ; then
head -n 2 $2 | tail -n 1 | tr "," " " | wc -w
exit 0
fi

if [ $1 == "-d" ] ; then
head -n 3 $2 | tail -n 1 | tr "," " " | wc -w
exit 0
fi

if [ $1 == "-p" ] ; then
head -n 1 $2 | tr "," "\n" | head -n 1
exit 0
fi

if [ $1 == "-n" ] ; then
head -n 1 $2 | tr ", " "\n" | tail -n 1
exit 0
fi

