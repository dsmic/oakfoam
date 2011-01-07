#!/bin/bash

cat | grep " 0x" | awk '{ count[$2] += $1 } END { for(elem in count) print count[elem], elem }'

