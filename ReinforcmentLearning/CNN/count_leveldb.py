"""
Generate data used in the HDF5DataLayer test.
"""
import os
import sys
import leveldb

db_file_name=sys.argv[1]

print "counting database: ",db_file_name

db = leveldb.LevelDB(db_file_name)

count=0
for key in db.RangeIter(include_value=False):
  count +=1

print "counted: ",count
