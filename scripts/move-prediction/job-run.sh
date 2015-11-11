#!/bin/bash

set -eu
set -o pipefail
OLDPWD="`pwd`"
cd `dirname "$0"`

if (( $# < 1 )); then
  echo "Missing parameters" >&2
  exit 1
fi

function cleanup
{
  rm -f "$PARAMS"
}

PARAMS="$1"
trap 'cleanup' ERR

./run.sh "$PARAMS"

cleanup

