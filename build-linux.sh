#!/bin/bash
# Boost for compiling 32-bit binaries on 64-bit:
#   ./bootstrap.sh
#   ./b2 link=static address-model=32 stage

set -eu

function boost-static
{
  sed -i 's/^\(oakfoam_LDADD =\) \(.*\) \($(HOARD_LIB).*\)$/\1 -Wl,-Bstatic \2 -Wl,-Bdynamic -pthread \3/' Makefile
}

VER=`cat config.h | sed -n 's/.*PACKAGE_VERSION \"\(.*\)\".*/\1/p'`
PREV_CONFIGURE=`cat config.log | head | sed -n 's/\s*$ //p'`
echo "configure was: $PREV_CONFIGURE"

DEBINPUT="0
oakfoam@gmail.com
5
BSD
6
games
7
i386
"

BOOST_ROOT=/data/opt/boost_1_47_0 $PREV_CONFIGURE --with-web 'CPPFLAGS=-m32' 'LDFLAGS=-m32 -pthread'
boost-static
echo "$DEBINPUT" | sudo checkinstall --nodoc --install=no make install
sudo chmod a+rw oakfoam oakfoam_*.deb

NAME=oakfoam_${VER}_i386

rm -f ${NAME}.tar.gz
mkdir ${NAME}

# BOOST_ROOT=/data/opt/boost_1_47_0 $PREV_CONFIGURE --with-web 'CPPFLAGS=-m32' 'LDFLAGS=-m32 -pthread'
# boost-static
make install DESTDIR=`pwd`/${NAME}

find ${NAME}/ -type f | grep -v 'menu\|applications\|www' | xargs -n1 -I{} mv {} $NAME/
find ${NAME}/ -type d -name www | xargs -n1 -I{} mv {} $NAME/

sed -i '/^cd \.\./d;/^bin=".*/d;s/$bin/\./' ${NAME}/oakfoam-web
mv ${NAME}/oakfoam-web ${NAME}/run.sh

tar -czf ${NAME}.tar.gz ${NAME}/
rm -r ${NAME}/

if [ "`uname -m`" == "x86_64" ]; then
  DEBINPUT="0
  oakfoam@gmail.com
  5
  BSD
  6
  games
  "

  $PREV_CONFIGURE --with-web
  boost-static
  make clean
  echo "$DEBINPUT" | sudo checkinstall --nodoc --install=no make install
  sudo chmod a+rw oakfoam oakfoam_*.deb

  NAME=oakfoam_${VER}_amd64

  rm -f ${NAME}.tar.gz
  mkdir ${NAME}

  # $PREV_CONFIGURE --with-web
  # boost-static
  make install DESTDIR=`pwd`/${NAME}

  find ${NAME}/ -type f | grep -v 'menu\|applications\|www' | xargs -n1 -I{} mv {} $NAME/
  find ${NAME}/ -type d -name www | xargs -n1 -I{} mv {} $NAME/

  sed -i '/^cd \.\./d;/^bin=".*/d;s/$bin/\./' ${NAME}/oakfoam-web
  mv ${NAME}/oakfoam-web ${NAME}/run.sh

  tar -czf ${NAME}.tar.gz ${NAME}/
  rm -r ${NAME}/
  make clean
fi

$PREV_CONFIGURE

