"""
Generate data used in the HDF5DataLayer test.
"""
import os
import numpy as np
import h5py
import sys
import leveldb
import caffe

first_line = sys.stdin.readline()

print "firstline "+ first_line +"\n"

num_rows = int(first_line)


num_inputs = 2
num_outputs = 1
height = 19
width = 19
total_size_in = num_inputs * height * width

data = np.arange(total_size_in)
data = data.reshape(num_inputs, height, width)
data = data.astype('float')

# We had a bug where data was copied into label, but the tests weren't
# catching it, so let's make label 1-indexed.

# label must be of same go board here
label = 1
#label = label.astype('float32')


db = leveldb.LevelDB('train_leveldb', create_if_missing=True, error_if_exists=True, write_buffer_size=268435456)
db_batch = leveldb.WriteBatch()

in_line=0
for line in sys.stdin:
  elements = line.split(',')
  if len(elements) != 19*19+1:
    print line
    continue
#  print elements[19*19]
  color=elements[19*19].split(':')
  c_played=0
  if color[0]=='B':
#	  print "black"
	  c_played=1
  if color[0]=='W':
#	  print "white"
	  c_played=2
  if c_played==0:
    print "ups, no move"
    print line
    continue
  pos=0
  for x in xrange(0,19):
	  for y in xrange(0,19):
		  #print "x: "+str(x)+" y: "+str(y)+" "+elements[pos]
		  n=int(elements[pos])
		  #print n
		  #print c_played
		  if c_played==1:			
			  if n==1:
				  data[0,x,y]=1
				  data[1,x,y]=0
			  elif n==2:
				  data[0,x,y]=0
				  data[1,x,y]=1
			  elif n==3 or n==4:
				  data[0,x,y]=0
				  data[1,x,y]=0
				  label=x*19+y
			  else:
				  data[0,x,y]=0
				  data[1,x,y]=0
		  if c_played==2:			
			  if n==1:
				  data[0,x,y]=0
				  data[1,x,y]=1
			  elif n==2:
				  data[0,x,y]=1
				  data[1,x,y]=0
			  elif n==3 or n==4:
				  data[0,x,y]=0
				  data[1,x,y]=0
				  label=x*19+y
			  else:
				  data[0,x,y]=0
				  data[1,x,y]=0
					
		  pos=pos+1
#  print data
  datum = caffe.proto.caffe_pb2.Datum()
  datum = caffe.io.array_to_datum(data,label)
#  datum.height = 19
#  datum.width = 19
#  datum.channels = 2
#  datum.label = label
#  datum.data = data.tostring()
  db_batch.Put('%08d' % in_line, datum.SerializeToString())
  if in_line % 1000 == 0:
    # Write batch of images to database
    db.Write(db_batch)
    del db_batch
    db_batch = leveldb.WriteBatch()
    print 'Processed %i position.' % in_line
 
  in_line=in_line+1

if in_line % 1000 != 0:
  # Write last batch of images
  db.Write(db_batch)

#with h5py.File(os.path.dirname(__file__) + '/sample_data.h5', 'w') as f:
#    f['data'] = data
#    f['label'] = label

#with open(os.path.dirname(__file__) + '/sample_data_list.txt', 'w') as f:
#    f.write(os.path.dirname(__file__) + '/sample_data.h5\n')



