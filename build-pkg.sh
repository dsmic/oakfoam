#!/bin/bash

INPUT="0
oakfoam@gmail.com
5
BSD
6
games
10
libboost-system-dev, libboost-filesystem-dev, libboost-thread-dev
"
echo "$INPUT" | sudo checkinstall --nodoc --install=no make install
chmod a+rw oakfoam oakfoam_*.deb

