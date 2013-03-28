#!/bin/bash

set -eu

cat | grep -v "^$" | awk '{ count[$2] += $1 } END { for(elem in count) print count[elem], elem }'

