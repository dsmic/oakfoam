#!/bin/bash

cd `dirname "$0"`
echo "Fetching the current opening book of Fuego"
wget http://fuego.svn.sourceforge.net/viewvc/fuego/trunk/book/book.dat
mv -i book.dat ../../
echo "book.dat moved to project root"

