#!/usr/bin/env python
# Read in a list of move prediction results and plot the results.
# The input file format is JSON (see plot_example.json).

import sys
import math
import json
import csv
import matplotlib
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import axes3d
from matplotlib import cm

jd = json.loads("".join(sys.stdin.readlines()))

plot3d = False
if 'type' in jd.keys():
  plot3d = jd['type']=='3d'

if plot3d:
  fig = plt.figure()

  ax = fig.add_subplot(111, projection='3d')

  fig.suptitle(jd['title'])
  ax.set_ylabel('Move Rank')
  ax.set_zlabel('Cumulative Probability')

  xmax = 200
  if 'xmax' in jd.keys():
    xmax = jd['xmax']

  X = []
  Y = []
  Z = []

  for f in jd['data']:
    with open(f['file'], 'rb') as csvfile:
      x = []
      y = []
      reader = csv.reader(csvfile)
      xi = 1
      s = 0
      for row in reader:
        rx = float(row[0])
        ry = float(row[1])
        if rx>xmax:
          s += ry
        else:
          while xi<rx:
            x.append(xi)
            y.append(0.0)
            xi += 1
          x.append(rx)
          y.append(ry)
          xi += 1

      s += sum(y)
      t = 0.0
      z = []
      zz = []
      for yy in y:
        t += yy
        z.append(t/s)
        zz.append(f['z'])

      Y.append(x)
      Z.append(z)
      X.append(zz)

  # ax.plot_wireframe(X, Y, Z, color='b', cmap='jet')
  surf = ax.plot_surface(X, Y, Z, rstride=1,  cstride=1, cmap=cm.coolwarm)
  fig.colorbar(surf, shrink=0.5, aspect=5)

  ax.set_zlim(0,1)
  ax.set_ylim(0,xmax)

else:
  plt.title(jd['title'])
  plt.xlabel('Move Rank')
  plt.ylabel('Cumulative Probability')
  plt.grid(True)
  plt.ylim(0,1)
  if 'xmax' in jd.keys():
    plt.xlim(0,float(jd['xmax']))
  else:
    plt.xlim(0,200)

  for f in jd['data']:
    with open(f['file'], 'rb') as csvfile:
      x = []
      y = []
      reader = csv.reader(csvfile)
      for row in reader:
        x.append(float(row[0]))
        y.append(float(row[1]))

      s = sum(y)
      t = 0.0
      z = []
      for yy in y:
        t += yy
        z.append(t/s)

      lbl = f['label'] + ' (%.1f%%)' % (z[0]*100)
      p = plt.plot(x, z, label = lbl)

  plt.legend(loc=4)

plt.show()

