#!/bin/bash

# Script for deploying Oakfoam to a testing server via SSH.
#   Oakfoam is SCP'ed to the relevant machine and compilation is attempted.
#
# Usage:
#   cd oakfoam
#   ./scripts/testing/deploy-ssh.sh <hostname>

set -u
HOST=$1
LINE="--------------------------------------------"

function init
{
  echo -en "$1..." | awk '{printf "%-40s",$0}' | tee -a deploy.log
}

function check
{
  if (($1==0)); then
    tput setaf 2
    echo Done | tee -a deploy.log
    tput sgr0
  else
    tput setaf 1
    echo Fail | tee -a deploy.log
    tput sgr0
    echo $LINE
    exit
  fi
}

echo $LINE | tee -a deploy.log
echo "Deploying to $HOST:" | tee -a deploy.log
echo "Changeset: `hg sum | sed -n 's/^parent: *//p'`" | tee -a deploy.log
date | tee -a deploy.log
echo $LINE | tee -a deploy.log

init "Archiving"
hg archive -t tgz oakfoam-dev.tar.gz 2>&1 >> deploy.log
check $?

init "Copying"
scp oakfoam-dev.tar.gz ${HOST}: 2>&1 >> deploy.log
check $?

init "Building"
ssh ${HOST} "tar xzf oakfoam-dev.tar.gz && cd oakfoam-dev/ && (make || (./configure && make)) && cd .. && (ln -s oakfoam-dev/oakfoam 2>/dev/null || true)" 2>&1 >> deploy.log
check $?

init "Checking default version"
VERSION=`ssh ${HOST} "echo 'describeengine' | ./oakfoam --nobook" 2>> deploy.log | head -n1 | sed 's/[^:]*: //'`
check $?
echo -en "Version:"
echo $VERSION | awk '{printf "%36s\n",$0}'

init "Cleaning"
rm -f oakfoam-dev.tar.gz
check $?

echo $LINE | tee -a deploy.log
rm -f deploy.log # ALL SUCCESS

