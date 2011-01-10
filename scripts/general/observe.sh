#!/bin/bash

tail -n`cat oakfoam.log | wc -l` -f oakfoam.log | ./observe.sed | gogui-display > /dev/null
