#!/bin/bash

set -u
OLDPWD=`pwd`
cd `dirname "$0"`

function msg
{
  echo "${1:-}" | tee -a $LOGFILE >&2
}

function init
{ 
  echo -en "${1:-Working}..." | awk '{printf "%-40s",$0}' | tee -a $LOGFILE >&2
  echo >> $LOGFILE
}

function check
{ 
  if ((${1:-0}==0)); then
    tput setaf 2
    msg "[Done]"
    tput sgr0
  else
    tput setaf 1
    msg "[Fail]"
    tput sgr0
    exit 1
  fi
}

RESDIR="results_`date +%F_%T`"
mkdir $RESDIR
cd $RESDIR
LOGFILE="test.log"

init "Loading parameters"
  if (( $# < 1 )); then
    echo "Missing parameter file" >> $LOGFILE
    check 1
  fi

  if ! test -e ${OLDPWD}/${1}; then
    echo "Parameter file '$1' not found" >> $LOGFILE
    check 1
  fi

  cp ${OLDPWD}/${1} params.test
  source ${OLDPWD}/${1}
check $?

INITGAMMAS="init.gamma"
touch $INITGAMMAS

# harvest patterns

if (( ${DT:-0} != 0 )); then
  DTFILE="dt.dat"
  init "Growing decision tree on $(echo "${DT_GAMES:-}" | wc -l) games"
  rm -f "$DTFILE" # clear
  ../../decisiontrees/dt-init.sh $DTFILE ${DT_FOREST_SIZE:-1} &>> $LOGFILE
  echo "${DT_GAMES:-}" | ../../decisiontrees/dt-grow.sh $DTFILE 1 &>> $LOGFILE
  check $?
  msg "Decision tree:"
  msg "  Forest size:   ${DT_FOREST_SIZE:-1}"
  msg "  Leaves:        `cat $DTFILE | grep 'WEIGHT' | wc -l`"
fi

TRAINEDGAMMAS="trained.gamma"

init "Training on $(echo "${TRAIN_GAMES:-}" | wc -l) games"
echo "${TRAIN_GAMES:-}" | ../../features/train-gammas.sh $INITGAMMAS large ${DTFILE:-} > $TRAINEDGAMMAS 2> $LOGFILE
check $?

rm -f $INITGAMMAS

# run tests

