"""
Generate data used in the HDF5DataLayer test.
"""
import os
import numpy as np
#import h5py
import sys
import leveldb
import caffe

db_file_name1=sys.argv[1]

#first_line = sys.stdin.readline()

#print "firstline "+ first_line +"\n"

#num_rows = int(first_line)
DIM=19
DIMm1=DIM-1

transforms_to_do=8
if (transforms_to_do != 8):
  print "attention, not all transformations generated!!!\n"

def transform(x,y,num):
  if num==0:
    return (x,y)
  elif num==1:
    return (DIMm1-x,y)
  elif num==2:
    return (x,DIMm1-y)
  elif num==3:
    return (DIMm1-x,DIMm1-y)
  elif num==4:
    return (y,x)
  elif num==5:
    return (DIMm1-y,x)
  elif num==6:
    return (y,DIMm1-x)
  elif num==7:
    return (DIMm1-y,DIMm1-x)
  

num_inputs = 23
num_outputs = 1
height = DIM
width = DIM
total_size_in = num_inputs * height * width

data = np.arange(total_size_in)
data = data.reshape(num_inputs, height, width)
data = data.astype('float')


use_only_win_moves=0
if (use_only_win_moves):
  print "Attention, only moves by the winning player are taken into account!!!\n"
test_stones_on_board=0
if (test_stones_on_board==1):
  print "Attention, stones played taken into account and not all boards are saved\n"
result_stones_on_board=200
if (result_stones_on_board>0):
  print "Attention, stones played taken into account for result\n"

# We had a bug where data was copied into label, but the tests weren't
# catching it, so let's make label 1-indexed.

# label must be of same go board here
label = 1
#label = label.astype('float32')


db = leveldb.LevelDB(db_file_name1, create_if_missing=True, error_if_exists=True, write_buffer_size=268435456)
db_batch = leveldb.WriteBatch()

in_line=0
for line in sys.stdin:
  elements = line.split(',')
  try:
#  if (1):
    if len(elements) != 3*DIM*DIM+20:
      print line
      continue
#  print elements[DIM*DIM]
    color=elements[3*DIM*DIM].split(':')
    c_played=0
    if color[0]=='B':
#	    print "black"
	    c_played=1
    if color[0]=='W':
#	    print "white"
	    c_played=2
    if c_played==0:
      print "ups, no move"
      print line
      continue
    x0=int(elements[3*DIM*DIM+1])
    y0=int(elements[3*DIM*DIM+2])
    x1=int(elements[3*DIM*DIM+3])
    y1=int(elements[3*DIM*DIM+4])
    x2=int(elements[3*DIM*DIM+5])
    y2=int(elements[3*DIM*DIM+6])
    x3=int(elements[3*DIM*DIM+7])
    y3=int(elements[3*DIM*DIM+8])
    x4=int(elements[3*DIM*DIM+9])
    y4=int(elements[3*DIM*DIM+10])
    x5=int(elements[3*DIM*DIM+11])
    y5=int(elements[3*DIM*DIM+12])
    x6=int(elements[3*DIM*DIM+13])
    y6=int(elements[3*DIM*DIM+14])
    xko=int(elements[3*DIM*DIM+15])
    yko=int(elements[3*DIM*DIM+16])
    result=elements[3*DIM*DIM+17]
    komi=elements[3*DIM*DIM+18]
    if ("Time" in result):
      #print "result by time"
      continue
    if ("Forfeit" in result):
      #print "result by time"
      continue
    if not ("B+" in result or "W+" in result):
      #print "no result or jigo ",result
      continue
    if not ("0.5" in komi or "6.5" in komi or "7.5" in komi or "3.75" in komi or "2.75" in komi or "5.5" in komi):
      #print "unsupported komi ",komi
      continue
    komiplane=0;
    if (c_played==1):
      if ("0.5" in komi):
        komiplane=1;
      if ("6.5" in komi or "2.75" in komi or "5.5" in komi): #komi 6.5 and 5.5 not very different in chinese scoring
        komiplane=2;
      if ("7.5" in komi or "3.75" in komi):
        komiplane=3;
    if (c_played==2):
      if ("0.5" in komi):
        komiplane=4;
      if ("6.5" in komi or "2.75" in komi or "5.5" in komi): #komi 6.5 and 5.5 not very different in chinese scoring
        komiplane=5;
      if ("7.5" in komi or "3.75" in komi):
        komiplane=6;
    komiplane = komiplane+13
    resultplane = 20
    resultis=0;
    if (c_played==1 and "B+" in result):
      resultis=1
    if (c_played==2 and "W+" in result):
      resultis=1
    if (use_only_win_moves==1 and resultis==0):
      #print "result ",result," color ",color
      continue
    #print komi," ",result," ",komiplane," ",resultis," ",c_played
    for transform_num in xrange(0,transforms_to_do):
      stones_on_board=0
      pos=0
      for xtmp in xrange(0,DIM):
        for ytmp in xrange(0,DIM):
          (x,y)=transform(xtmp,ytmp,transform_num)
          for k in range(0,num_inputs):
            data[k,x,y]=0
            if komiplane > 13 and k== komiplane:
              data[k,x,y]=1
            if k == resultplane:
              data[k,x,y]=resultis         
          #print "x: "+str(x)+" y: "+str(y)+" "+elements[pos]
          n=int(elements[pos])
          libs=int(elements[DIM*DIM+pos])
          if libs>4:
            libs=4
          libs=libs-1
          if libs<0 and n>0 and n!=3 and n!=4: print "libs wrong "+ str(libs) + "--" +str(n)
          if n!=1 and n!=2: 
            data[8,x,y]=1
          else:
            stones_on_board+=1
          #stones=int(elements[2*DIM*DIM+pos])
          #print n
          #print c_played
          if c_played==1:			
            if n==1:
              #data[0,x,y]=1
              if libs>=0: data[0+libs,x,y]=1
              #print "X",
            elif n==2:
              #data[1,x,y]=1
              if libs>=0: data[4+libs,x,y]=1
              #print "O",
            #elif n==3 or n==4:
              #label=x*DIM+y
              #print ".",
            #else:
              #print ".",
            
          if c_played==2:			
            if n==1:
              #data[1,x,y]=1
              if libs>=0: data[4+libs,x,y]=1
              #print "X",
            elif n==2:
              #data[0,x,y]=1
              if libs>=0: data[0+libs,x,y]=1
              #print "O",
            #elif n==3 or n==4:
              #label=x*DIM+y
              #print ".",
            #else:
              #print ".",
          pos=pos+1
        #print ""
      #print "move",x2,y2
      
      if (test_stones_on_board==1 and stones_on_board < 70):
        continue
      if (test_stones_on_board==1 and stones_on_board <150):
        continue
      resultisnew=resultis
      if (result_stones_on_board >0):
        #print "ok",resultis,stones_on_board
        if (stones_on_board<result_stones_on_board):
          resultisnew=(resultis-0.5)*(float(stones_on_board)/result_stones_on_board)+0.5
        #print resultisnew
      for x in xrange(0,DIM):
        for y in xrange(0,DIM): 
          data[resultplane,x,y]=resultisnew
      if (x1>=0 and y1>=0):
        (x1n,y1n)=transform(x1,y1,transform_num)
        data[21,x1n,y1n]=1
      if (x0>=0 and y0>=0): 
        (x0n,y0n)=transform(x0,y0,transform_num)
        data[22,x0n,y0n]=1
      if (xko>=0 and yko>=0): 
        (xkon,ykon)=transform(xko,yko,transform_num)
        data[9,xkon,ykon]=1
      if (x3>=0 and y3>=0): 
        (x3n,y3n)=transform(x3,y3,transform_num)
        data[10,x3n,y3n]=1
      if (x4>=0 and y4>=0): 
        (x4n,y4n)=transform(x4,y4,transform_num)
        data[11,x4n,y4n]=1
      if (x5>=0 and y5>=0): 
        (x5n,y5n)=transform(x5,y5,transform_num)
        data[12,x5n,y5n]=1
      if (x6>=0 and y6>=0): 
        (x6n,y6n)=transform(x6,y6,transform_num)
        data[13,x6n,y6n]=1
      #data2=data[:-2,:,:]
      if (x2>=0 and y2>=0):
        (x2n,y2n)=transform(x2,y2,transform_num)
        label=x2n*DIM+y2n
#        datum = caffe.proto.caffe_pb2.Datum()
        datum = caffe.io.array_to_datum(data,label)
        db_batch.Put('%09d' % in_line, datum.SerializeToString())
        if in_line % 1000 == 0:
          # Write batch of images to database
          db.Write(db_batch)
          del db_batch
          db_batch = leveldb.WriteBatch()
          print 'Processed %i position.' % in_line
        in_line=in_line+1

  except:
#  if (0):
    print 'try exception accured'

if in_line % 1000 != 0:
  # Write last batch of images
  db.Write(db_batch)

#with h5py.File(os.path.dirname(__file__) + '/sample_data.h5', 'w') as f:
#    f['data'] = data
#    f['label'] = label

#with open(os.path.dirname(__file__) + '/sample_data_list.txt', 'w') as f:
#    f.write(os.path.dirname(__file__) + '/sample_data.h5\n')



