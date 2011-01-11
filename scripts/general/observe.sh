#!/bin/bash

tail -n`cat oakfoam.log | wc -l` -F oakfoam.log | ./observe.sed | gogui-display > /dev/null
