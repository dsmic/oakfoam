"""
Generate data used in the HDF5DataLayer test.
"""
import os
import sys
import leveldb

db_file_name=[]
db=[]

num_databases=len(sys.argv)

for i in range(1,num_databases):
  print i,sys.argv[i]
  db.append(leveldb.LevelDB(sys.argv[i]))

count_0=0
for key in db[0].RangeIter(include_value=False):
#  print "#",key,"#"
  count_0 +=1

print "counted 0: ",count_0

count=0

for i in range(1,num_databases-1):
  for (key,value) in db[i].RangeIter():
    db[0].Put('%09d' % count_0,value)
    count+=1
    count_0+=1

print "counted: ",count
