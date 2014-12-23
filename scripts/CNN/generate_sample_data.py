"""
Generate data used in the HDF5DataLayer test.
"""
import os
import numpy as np
import h5py
import sys

first_line = sys.stdin.readline()

print "firstline "+ first_line +"\n"

num_rows = int(first_line)


num_inputs = 2
num_outputs = 1
height = 19
width = 19
total_size_in = num_inputs * num_rows * height * width
total_size_out = num_outputs * num_rows * height * width

data = np.arange(total_size_in)
data = data.reshape(num_rows, num_inputs, height, width)
data = data.astype('float32')

# We had a bug where data was copied into label, but the tests weren't
# catching it, so let's make label 1-indexed.

# label must be of same go board here
data2 = np.arange(total_size_out)
data2 = data2.reshape(num_rows, num_outputs, height, width)
data2 = data2.astype('float32')

label = 1 + np.arange(num_rows)[:, np.newaxis]
label = label.astype('float32')

in_line=0
for line in sys.stdin:
	elements = line.split(',')
	print line;
	print elements[19*19]
	color=elements[19*19].split(':')
	c_played=0
	if color[0]=='B':
		print "black"
		c_played=1
	if color[0]=='W':
		print "white"
		c_played=2
	pos=0
	for x in xrange(0,19):
		for y in xrange(0,19):
			#print "x: "+str(x)+" y: "+str(y)+" "+elements[pos]
			n=int(elements[pos])
			#print n
			#print c_played
			if c_played==1:			
				if n==1:
					data[in_line,0,x,y]=1
					data[in_line,1,x,y]=0
					data2[in_line,0,x,y]=0
				elif n==2:
					data[in_line,0,x,y]=0
					data[in_line,1,x,y]=1
					data2[in_line,0,x,y]=0
				elif n==3 or n==4:
					data[in_line,0,x,y]=0
					data[in_line,1,x,y]=0
					data2[in_line,0,x,y]=1
				else:
					data[in_line,0,x,y]=0
					data[in_line,1,x,y]=0
					data2[in_line,0,x,y]=0
			if c_played==2:			
				if n==1:
					data[in_line,0,x,y]=0
					data[in_line,1,x,y]=1
					data2[in_line,0,x,y]=0
				elif n==2:
					data[in_line,0,x,y]=1
					data[in_line,1,x,y]=0
					data2[in_line,0,x,y]=0
				elif n==3 or n==4:
					data[in_line,0,x,y]=0
					data[in_line,1,x,y]=0
					data2[in_line,0,x,y]=1
				else:
					data[in_line,0,x,y]=0
					data[in_line,1,x,y]=0
					data2[in_line,0,x,y]=0
						
			pos=pos+1
	in_line=in_line+1


for x in xrange(0,19):
	for y in xrange(0,19):
		print(str(data[3,0,x,y])),
	print
print
for x in xrange(0,19):
	for y in xrange(0,19):
		print(str(data[3,1,x,y])),
	print
print
for x in xrange(0,19):
	for y in xrange(0,19):
		print(str(data2[3,0,x,y])),
	print
print			
	
print num_rows
print data
print label

with h5py.File(os.path.dirname(__file__) + '/sample_data.h5', 'w') as f:
    f['data'] = data
    f['label'] = label

with h5py.File(os.path.dirname(__file__) + '/sample_data2.h5', 'w') as f:
    f['data'] = data2
    f['label'] = label

#with h5py.File(os.path.dirname(__file__) + '/sample_data_2_gzip.h5', 'w') as f:
#    f.create_dataset(
#        'data', data=data,
#        compression='gzip', compression_opts=1
#    )
#    f.create_dataset(
#        'label', data=label,
#        compression='gzip', compression_opts=1
#    )

with open(os.path.dirname(__file__) + '/sample_data_list.txt', 'w') as f:
    f.write(os.path.dirname(__file__) + '/sample_data.h5\n')
#    f.write(os.path.dirname(__file__) + '/sample_data_2_gzip.h5\n')

with open(os.path.dirname(__file__) + '/sample_data_list2.txt', 'w') as f:
    f.write(os.path.dirname(__file__) + '/sample_data2.h5\n')



