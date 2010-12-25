#!/bin/bash

sort | uniq -c | sort -rn | grep -nT ""

