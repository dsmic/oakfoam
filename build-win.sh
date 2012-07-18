#!/bin/bash

# This script is to help me with cross compiling for Windows on my machine.
# It would be better if the boost folder was detected and not just local to my system.
#   -- Francois van Niekerk

set -eu

VER=`cat config.h | sed -n 's/.*PACKAGE_VERSION \"\(.*\)\".*/\1/p'`

BOOST_ROOT=/data/win/dev/boost_1_47_0 ./configure --host=i586-mingw32msvc --build=`./config.guess` --with-web
make clean
make
strip oakfoam.exe

mkdir oakfoam_${VER}_win32
cp oakfoam.exe oakfoam_${VER}_win32/
cp *.dll oakfoam_${VER}_win32/
cp book.dat oakfoam_${VER}_win32/
cp -R www/ oakfoam_${VER}_win32/

echo '@echo off' > oakfoam_${VER}_win32/run.bat
echo 'start /b oakfoam --web -c settings.gtp' >> oakfoam_${VER}_win32/run.bat
echo 'start http://localhost:8000' >> oakfoam_${VER}_win32/run.bat
echo 'pause' >> oakfoam_${VER}_win32/run.bat

echo 'param playouts_per_move 10000000' > oakfoam_${VER}_win32/settings.gtp
echo 'param time_move_max 5.00' >> oakfoam_${VER}_win32/settings.gtp
echo 'param rules_all_stones_alive 0' >> oakfoam_${VER}_win32/settings.gtp

rm -rf oakfoam_${VER}_win32.zip
zip -r oakfoam_win32 oakfoam_${VER}_win32
mv oakfoam_win32.zip oakfoam_${VER}_win32.zip
rm -r oakfoam_${VER}_win32/

./configure --with-web
make clean
make


