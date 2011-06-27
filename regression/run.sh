#!/bin/bash

#########################################
# Oakfoam Regression Testing
#########################################
# Usage: ./run.sh [TEST|@TESTSUITE]...
#########################################

OUTPUT="results_`date +%F_%T`"
OAKFOAM="../oakfoam --nobook --log $OUTPUT/oakfoam.log"
PROGRAM="gogui-adapter \"$OAKFOAM\""
# Use gogui-adapter to emulate loadsgf

if ! test -x ../oakfoam; then
  echo "File ../oakfoam not found" >&2
  exit 1
fi

if (( $# == 0 )); then
  TESTS="@all.suite"
  echo "No test(s)/test suite(s) supplied. Running test: '$TESTS'."
else
  TESTS=$@
fi

mkdir -p $OUTPUT
echo "Saving output to '$OUTPUT'..."

echo "describeengine" | $OAKFOAM > /dev/null

gogui-regress -long -output $OUTPUT "$PROGRAM" $TESTS
