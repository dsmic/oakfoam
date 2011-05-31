#! /bin/bash

valgrind --tool=callgrind --dump-instr=yes ./oakfoam --nobook -c profile.gtp
