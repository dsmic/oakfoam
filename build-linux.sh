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
sudo chmod a+rw oakfoam oakfoam_*.deb

VER=`cat config.h | sed -n 's/.*PACKAGE_VERSION \"\(.*\)\".*/\1/p'`
PREV_CONFIGURE=`cat config.log | head | sed -n 's/\s*$ //p'`
NAME=oakfoam_${VER}_i386

rm -f ${NAME}.tar.gz
mkdir ${NAME}
$PREV_CONFIGURE --prefix=`pwd`/${NAME}
make install
mv ${NAME}/bin/* ${NAME}/
rm -r ${NAME}/bin/
mv ${NAME}/share/oakfoam/* ${NAME}/
rm -r ${NAME}/share/
sed -i '/^cd \.\./d;/^bin="/d;s/$bin/\./' ${NAME}/oakfoam-web
mv ${NAME}/oakfoam-web ${NAME}/run.sh
tar -czf ${NAME}.tar.gz ${NAME}/
rm -r ${NAME}/

$PREV_CONFIGURE

