#!/bin/bash
#
# ./multi-core.sh <boardsize> <playouts> <threads> <virtualloss> <lockfree> <samples>
#

GTP="boardsize $1
param playouts_per_move $2
param uct_virtual_loss $4
param uct_lock_free $5"

echo "Running benchmark for $3 threads on $1x$1 (plts:$2 vl:$4 lf:$5)"

for i in `seq 1 $3`; do
  echo -n "$i:"
  ALLPPMS=""
  for j in `seq 1 $6`; do
    PPMS=`echo -e "param thread_count $i\n$GTP\ngenmove b" | ./oakfoam --nobook 2>&1 | sed -n 's/^.*ppms:\([0-9.]*\).*$/\1/p'`
    echo -en "\t$PPMS"
    ALLPPMS="$ALLPPMS\n$PPMS"
  done
  MAX=`echo -e "$ALLPPMS" | sort -rn | head -n1`
  echo -e "\tmax: $MAX"
done

