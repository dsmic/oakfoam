#!/bin/bash

set -eu
set -o pipefail
OLDPWD="`pwd`"
cd `dirname "$0"`

if ! test -x ./job-queue.sh; then
  echo "Job queue script not found or executable" >&2
  echo "Refer to job-queue.sh.example" >&2
  exit 1
fi

if (( $# < 1 )); then
  echo "Missing parameter file" >&2
  exit 1
fi

JOB="${OLDPWD}/${1}"

if ! test -e "$JOB"; then
  echo "Parameter file '$JOB' not found" >&2
  exit 1
fi

NEWJOB="`mktemp params_job_XXXX.test`"
echo "Copying parameters to: $NEWJOB"
cp "$JOB" "$NEWJOB"

CMD="`pwd`/job-run.sh $NEWJOB"

echo "Queuing job: $CMD"

./job-queue.sh "$CMD"

echo "Job queued."


