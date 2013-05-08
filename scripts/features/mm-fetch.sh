#!/bin/bash

wget http://remi.coulom.free.fr/Amsterdam2007/mm.tar.bz2
tar -xjf mm.tar.bz2
patch -p 0 <mm.patch
cd mm
make
cd ..
