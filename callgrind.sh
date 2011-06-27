#! /bin/bash

valgrind --tool=callgrind --dump-instr=yes --instr-atstart=no ./oakfoam --nobook -c profile.gtp & (sleep 2.5; callgrind_control -i on)

