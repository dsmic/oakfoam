#! /bin/bash

valgrind --tool=callgrind ./oakfoam -c profile.gtp
