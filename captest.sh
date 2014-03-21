#!/bin/bash
make canonicalizeplanargraph && make canonicalizeplanargraph-nole && make main3

## (01)^K

## Merk op dat indien canonicalizeplanargraph zou falen of crashen, we geen fout zouden merken in de uitvoer van dit script!

MINK=4
MAXK=14

CAPDIR=`pwd`/caps
OUTPUT_DIR=`pwd`/out/

####
TUBETYPE=${CAPDIR}/tubetype
K=$MINK

MAIN3FILE=${OUTPUT_DIR}/main3_caps_${K}_out
TTFILE=${OUTPUT_DIR}/tubetype_caps_${K}_out

# &> redirect zowel stdout als stderr

while (( "$K" <= "$MAXK" ))
do
    # Safety
    rm -f $MAIN3FILE
    rm -f $TTFILE
    rm -f ${OUTPUT_DIR}/easycompare_${K}_main3_canonicalized
    rm -f ${OUTPUT_DIR}/easycompare_${K}_tt_canonicalized
    rm -f ${OUTPUT_DIR}/easycompare_${K}_main3_canonicalized_sort
    rm -f ${OUTPUT_DIR}/easycompare_${K}_tt_canonicalized_sort

    ./main3 $MAIN3FILE $K borders_01_k &> /dev/null
    $TUBETYPE $K 0 > $TTFILE 2> /dev/null

    ./canonicalizeplanargraph $MAIN3FILE ${OUTPUT_DIR}/easycompare_${K}_main3_canonicalized nopairs &> /dev/null
    ./canonicalizeplanargraph $TTFILE ${OUTPUT_DIR}/easycompare_${K}_tt_canonicalized nopairs &> /dev/null

    sort ${OUTPUT_DIR}/easycompare_${K}_main3_canonicalized > ${OUTPUT_DIR}/easycompare_${K}_main3_canonicalized_sort
    sort ${OUTPUT_DIR}/easycompare_${K}_tt_canonicalized > ${OUTPUT_DIR}/easycompare_${K}_tt_canonicalized_sort
    diff ${OUTPUT_DIR}/easycompare_${K}_main3_canonicalized_sort ${OUTPUT_DIR}/easycompare_${K}_tt_canonicalized_sort &> /dev/null

    echo "Tested k=$K resultaat: " $?

    ((K += 1))
done
