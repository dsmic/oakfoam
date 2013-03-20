#!/bin/bash

# stdin: oakfoam.log file
# out: places where a surewin was 'bad'
grep -n "\[genmove\]:\|clear" | grep -A1 "surewin" | grep -v -B1 "surewin\|clear\|--"
