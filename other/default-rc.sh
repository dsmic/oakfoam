#!/bin/bash
#######################################
# List the default parameters in GTP
# format, excluding the random seed.
#######################################

if ! test -x ../oakfoam; then
  echo "File ../oakfoam not found" >&2
  exit 1
fi

PARAMS=`echo "describeengine" | ../oakfoam | sed -n "s/^ *\([^ ]*\) [ ]*\([^ ]*\) *$/param \1 \2/p"`

echo "$PARAMS" | grep -ve "^param rand_seed "
