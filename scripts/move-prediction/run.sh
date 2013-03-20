#!/bin/bash

set -u
OLDPWD=`pwd`
cd `dirname "$0"`

function msg_short
{
  echo "${1:-}" | tee -a $LOGFILE >&2
}

function msg
{
  DATE=`date +%F_%T`
  msg_short "[$DATE] ${1:-}"
}

function init
{ 
  DATE=`date +%F_%T`
  echo -en "[$DATE] ${1:-Working}..." | awk '{printf "%-70s",$0}' | tee -a $LOGFILE >&2
  echo >> $LOGFILE
}

function check
{ 
  if ((${1:-0}==0)); then
    tput setaf 2
    msg_short "[Done]"
    tput sgr0
  else
    tput setaf 1
    msg_short "[Fail]"
    tput sgr0
    exit 1
  fi
}

function lastline
{
  tput sc
  while read LINE; do
    tput rc
    tput sc
    tput el
    echo "$LINE" >> $LOGFILE
    echo -n "$LINE" >&2
    sleep 0.1 # flushes output
    # sync
  done
  tput rc
  tput el
}

RESDIR="results_`date +%F_%T`"
mkdir $RESDIR
cd $RESDIR
LOGFILE="test.log"
msg "Writting results to '`pwd`'"

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
  init "Growing decision forest on $(echo "${DT_GAMES:-}" | wc -l) games"
  rm -f "$DTFILE" # clear
  ../../decisiontrees/dt-init.sh $DTFILE ${DT_FOREST_SIZE:-1} &>> $LOGFILE
  exec 3<> tmp
  echo "${DT_GAMES:-}" | ../../decisiontrees/dt-grow.sh $DTFILE 1 2>&1 1>&3 | lastline
  check $?
  msg "Decision forest:"
  msg "  Forest size:   ${DT_FOREST_SIZE:-1}"
  msg "  Leaves:        `cat $DTFILE | grep 'WEIGHT' | wc -l`"
fi

TRAINEDGAMMAS="trained.gamma"

init "Training on $(echo "${TRAIN_GAMES:-}" | wc -l) games"
# echo "${TRAIN_GAMES:-}" | ../../features/train-gammas.sh $INITGAMMAS large ${DTFILE:-} > $TRAINEDGAMMAS 2>> $LOGFILE
(echo "${TRAIN_GAMES:-}" | ../../features/train-gammas.sh $INITGAMMAS large ${DTFILE:-} > $TRAINEDGAMMAS) 2>&1 | lastline
check $?

rm -f $INITGAMMAS

# run tests

