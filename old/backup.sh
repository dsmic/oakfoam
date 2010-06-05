#! /bin/bash

name="backup_`date +%F_%T`.tar.gz"

mkdir backup
cp *.c backup/
cp *.h backup/ 2>/dev/null
cp *.txt backup/ 2>/dev/null
cp makefile backup/

cd backup
tar -c * | gzip -cf9 - > $name
cd ..

mv backup/$name backups/
rm -f -R backup


