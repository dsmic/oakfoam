#!/bin/bash

if (( $# != 2 )); then
  echo "Exactly 2 parameters required!" >&2
  exit 1
fi
awk '{ count[$2] += $1 } END { for(elem in count) print count[elem], elem }' $1 $2

