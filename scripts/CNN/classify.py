#!/usr/bin/env python
"""
classify.py is an out-of-the-box image classifer callable from the command line.

By default it configures and runs the Caffe reference ImageNet model.
"""
import numpy as np
import os
import sys
import argparse
import glob
import time

import caffe

class Classifier(caffe.Net):
    """
    Classifier extends Net for image class prediction
    by scaling, center cropping, or oversampling.
    """
    def __init__(self, model_file, pretrained_file, image_dims=None,
                 gpu=False, mean=None, input_scale=None, raw_scale=None,
                 channel_swap=None):
        """
        Take
        image_dims: dimensions to scale input for cropping/sampling.
            Default is to scale to net input size for whole-image crop.
        gpu, mean, input_scale, raw_scale, channel_swap: params for
            preprocessing options.
        """
        caffe.Net.__init__(self, model_file, pretrained_file)
        self.set_phase_test()

        if gpu:
            self.set_mode_gpu()
        else:
            self.set_mode_cpu()

        if mean is not None:
            self.set_mean(self.inputs[0], mean)
        if input_scale is not None:
            self.set_input_scale(self.inputs[0], input_scale)
        if raw_scale is not None:
            self.set_raw_scale(self.inputs[0], raw_scale)
        if channel_swap is not None:
            self.set_channel_swap(self.inputs[0], channel_swap)

        self.crop_dims = np.array(self.blobs[self.inputs[0]].data.shape[2:])
        if not image_dims:
            image_dims = self.crop_dims
        self.image_dims = image_dims


    def predict(self, inputs, oversample=True):
        """
        Predict classification probabilities of inputs.

        Take
        inputs: iterable of (H x W x K) input ndarrays.
        oversample: average predictions across center, corners, and mirrors
                    when True (default). Center-only prediction when False.

        Give
        predictions: (N x C) ndarray of class probabilities
                     for N images and C classes.
        """
        # Scale to standardize input dimensions.
        #input_ = np.zeros((len(inputs),
        #    self.image_dims[0], self.image_dims[1], inputs[0].shape[2]),
        #    dtype=np.float32)
        #for ix, in_ in enumerate(inputs):
        #    input_[ix] = caffe.io.resize_image(in_, self.image_dims)

        #if oversample:
        #    # Generate center, corner, and mirrored crops.
        #    input_ = caffe.io.oversample(input_, self.crop_dims)
        #else:
        #    # Take center crop.
        #    center = np.array(self.image_dims) / 2.0
        #    crop = np.tile(center, (1, 2))[0] + np.concatenate([
        #        -self.crop_dims / 2.0,
        #        self.crop_dims / 2.0
        #    ])
        #    input_ = input_[:, crop[0]:crop[2], crop[1]:crop[3], :]

        # Classify
        #caffe_in = np.zeros(np.array(input_.shape)[[0,3,1,2]],
        #                    dtype=np.float32)
        #for ix, in_ in enumerate(input_):
        #    caffe_in[ix] = self.preprocess(self.inputs[0], in_)
        out = self.forward_all(**{'data': inputs})
        predictions = out

        return predictions

def main(argv):
    pycaffe_dir = os.path.dirname(__file__)

    parser = argparse.ArgumentParser()
    # Required arguments: input and output files.
    parser.add_argument(
        "input_file",
        help="Input image, directory, or npy."
    )
    parser.add_argument(
        "output_file",
        help="Output npy filename."
    )
    # Optional arguments.
    parser.add_argument(
        "--model_def",
        default=os.path.join(pycaffe_dir,
                "lenet.prototxt"),
        help="Model definition file."
    )
    parser.add_argument(
        "--pretrained_model",
        default=os.path.join(pycaffe_dir,
                "snapshots/_iter_5000.caffemodel"),
        help="Trained model weights file."
    )
    parser.add_argument(
        "--gpu",
        action='store_true',
        help="Switch for gpu computation."
    )
    parser.add_argument(
        "--images_dim",
        default='256,256',
        help="Canonical 'height,width' dimensions of input images."
    )
    args = parser.parse_args()

    image_dims = [int(s) for s in args.images_dim.split(',')]

    # Make classifier.
    classifier = Classifier(args.model_def, args.pretrained_model)

    if args.gpu:
        print 'GPU mode'

    # Load numpy array (.npy), directory glob (*.jpg), or image file.
    args.input_file = os.path.expanduser(args.input_file)
    
    f_input = open(args.input_file, 'r')
    
    first_line = f_input.readline()
    numlines = int(first_line)
    nummatched =0    

    num_rows = 1


    num_inputs = 2
    num_outputs = 1
    height = 19
    width = 19
    total_size_in = num_inputs * num_rows * height * width
    total_size_out = num_outputs * num_rows * height * width
	
    data = np.arange(total_size_in)
    data = data.reshape(num_rows, num_inputs, height, width)
    data = data.astype('float32')

    data2 = np.arange(total_size_out)
    data2 = data2.reshape(num_rows, num_outputs, height, width)
    data2 = data2.astype('float32')

    for linenr in xrange(0,numlines):
        line = f_input.readline()
        elements = line.split(',')
        #print line;
        print
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
        in_line=0
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
	


        # Classify.
        start = time.time()
        predictions = classifier.predict(data)
        #print "OK"
        #print predictions
        #print "Done in %.2f s." % (time.time() - start)
        for x in xrange(0,19):
            for y in xrange(0,19):
                if data[in_line,0,x,y] >0: print "{:5.2f}".format(1.0),
                elif data[in_line,1,x,y] >0: print "{:5.2f}".format(-1.0),
                else: print "{:5.2f}".format(0.0),
            print
        print        
        sum=0
        maxpred=0;
        for x in xrange(0,19):
            for y in xrange(0,19):
                sum=sum+predictions['ip'][0,x*19+y,0,0]
                print "{:5.2f}".format(predictions['ip'][0,x*19+y,0,0]),
                if maxpred<predictions['ip'][0,x*19+y,0,0]:
                    xp=x
                    yp=y
                    maxpred=predictions['ip'][0,x*19+y,0,0]
                if data2[in_line,0,x,y] >0:
                    xis=x
                    yis=y
            print
        matched=0
        if xp==xis and yp==yis: 
            matched=1
            nummatched=nummatched+1
        print sum,xp,yp,xis,yis
    print nummatched,numlines

    # Save
    np.save(args.output_file, predictions)


if __name__ == '__main__':
    main(sys.argv)
