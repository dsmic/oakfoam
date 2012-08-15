#!/bin/bash

cat | grep -v "^$" | gawk '{ count[$2] += $1 } END { for(elem in count) print count[elem], elem }'

