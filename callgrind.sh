#! /bin/bash

valgrind --tool=callgrind --dump-instr=yes ./oakfoam -c profile.gtp
