#!/bin/bash
make canonicalizeplanargraph && make canonicalizeplanargraph-nole && make main3

## Merk op dat indien canonicalizeplanargraph zou falen of crashen, we geen fout zouden merken in de uitvoer van dit script!

MINLENGTH=12
MAXLENGTH=22
MINPENTAGONS=2
MAXPENTAGONS=4

#MODE="ISO_PAIRS"
#MODE="GROWTH_PAIRS"
MODE="ISO_PATCHES"

BOUNDARY_DIR=`pwd`/boundary/
OUTPUT_DIR=`pwd`/out/

####################
LENGTH=$MINLENGTH
BOUNDARY_EXECUTABLE=${BOUNDARY_DIR}/boundary
CURDIR=`pwd`

# &> redirect zowel stdout als stderr

while (( "$LENGTH" <= "$MAXLENGTH" ))
do
  PENTAGONS=$MINPENTAGONS
  while (( "$PENTAGONS" <= "$MAXPENTAGONS" ))
  do
    if [ "$MODE" == "ISO_PAIRS" ]
    then
      BASE_MAIN3_FILENAME="${OUTPUT_DIR}/easycompare_${LENGTH}_${PENTAGONS}_main3_isopairs"
      BASE_BD_FILENAME="${BOUNDARY_DIR}/bd${LENGTH}_pent${PENTAGONS}_isomerisation_pairs.pl"
      ./main3 $BASE_MAIN3_FILENAME ${LENGTH} ${LENGTH} ${PENTAGONS} pairs_out isomerisation &> /dev/null
      cd ${BOUNDARY_DIR}
      ${BOUNDARY_EXECUTABLE} ${LENGTH} ${PENTAGONS} pairs_out isomerisation &> /dev/null
      cd ${CURDIR}
    elif [ "$MODE" == "ISO_PATCHES" ]
    then
      BASE_MAIN3_FILENAME="${OUTPUT_DIR}/easycompare_${LENGTH}_${PENTAGONS}_main3_isopatches"
      BASE_BD_FILENAME="${BOUNDARY_DIR}/bd${LENGTH}_pent${PENTAGONS}_diff_gluings_of_same_patch.pl"
      ./main3 $BASE_MAIN3_FILENAME ${LENGTH} ${LENGTH} ${PENTAGONS} pairs_out isopatches &> /dev/null
      cd ${BOUNDARY_DIR}
      ${BOUNDARY_EXECUTABLE} ${LENGTH} ${PENTAGONS} pairs_out isomerisation &> /dev/null
      cd ${CURDIR}
    else
      BASE_MAIN3_FILENAME="${OUTPUT_DIR}/easycompare_${LENGTH}_${PENTAGONS}_main3_growthpairs"
      BASE_BD_FILENAME="${BOUNDARY_DIR}/bd${LENGTH}_pent${PENTAGONS}_growth_pairs.pl"
      ./main3 $BASE_MAIN3_FILENAME ${LENGTH} ${LENGTH} ${PENTAGONS} pairs_out growth &> /dev/null
      cd ${BOUNDARY_DIR}
      ${BOUNDARY_EXECUTABLE} ${LENGTH} ${PENTAGONS} pairs_out growth &> /dev/null
      cd ${CURDIR}
    fi
    if [ "$MODE" == "ISO_PATCHES" ]
    then
      ./canonicalizeplanargraph $BASE_MAIN3_FILENAME ${OUTPUT_DIR}/easycompare_${LENGTH}_${PENTAGONS}_main3_canonicalized nopairs &> /dev/null
      ./canonicalizeplanargraph-nole $BASE_BD_FILENAME ${OUTPUT_DIR}/easycompare_${LENGTH}_${PENTAGONS}_bd_canonicalized nopairs &> /dev/null
    else
      ./canonicalizeplanargraph $BASE_MAIN3_FILENAME ${OUTPUT_DIR}/easycompare_${LENGTH}_${PENTAGONS}_main3_canonicalized &> /dev/null
      ./canonicalizeplanargraph-nole $BASE_BD_FILENAME ${OUTPUT_DIR}/easycompare_${LENGTH}_${PENTAGONS}_bd_canonicalized &> /dev/null
    fi
    sort ${OUTPUT_DIR}/easycompare_${LENGTH}_${PENTAGONS}_main3_canonicalized > ${OUTPUT_DIR}/easycompare_${LENGTH}_${PENTAGONS}_main3_canonicalized_sort
    sort ${OUTPUT_DIR}/easycompare_${LENGTH}_${PENTAGONS}_bd_canonicalized > ${OUTPUT_DIR}/easycompare_${LENGTH}_${PENTAGONS}_bd_canonicalized_sort
    diff ${OUTPUT_DIR}/easycompare_${LENGTH}_${PENTAGONS}_main3_canonicalized_sort ${OUTPUT_DIR}/easycompare_${LENGTH}_${PENTAGONS}_bd_canonicalized_sort &> /dev/null

    echo "Tested l=$LENGTH p=$PENTAGONS resultaat: " $?
    ((PENTAGONS += 1))
  done
  ((LENGTH += 1))
done
