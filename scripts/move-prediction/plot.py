#!/usr/bin/env python
# Read in a list of move prediction results and plot the results.
# The input file format is JSON (see plot_example.json).

import sys
import math
import numpy as np
import json
import csv
import matplotlib
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import axes3d
from matplotlib import cm

jd = json.loads("".join(sys.stdin.readlines()))

plottype = jd['type']

if plottype == 'move-prediction': # 2d move prediction
  fig = plt.figure()
  fig.canvas.set_window_title(jd['title']) 
  plt.title(jd['title'])
  plt.xlabel('Move Rank')
  plt.ylabel('Cumulative Probability')
  plt.grid(True)
  plt.ylim(0,1)
  xmin = 1
  xmax = 50
  if 'xmax' in jd.keys():
    xmax = int(jd['xmax'])
  plt.xlim(xmin,xmax)
  plt.yticks(np.append(np.arange(0,1,0.05),1))
  plt.xticks(np.append(np.arange(xmin,xmax),xmax))
  errk = 0.0
  if 'errk' in jd.keys():
    errk = float(jd['errk'])

  table = []
  table.append(range(xmin,xmax+1))
  tablelabels = []
  tablelabels.append('Move Rank')

  c = 0
  for f in jd['data']:
    with open(f['file'], 'rb') as csvfile:
      x = []
      y = []
      z = []
      reader = csv.reader(csvfile)
      for row in reader:
        x.append(float(row[0]))
        y.append(float(row[1]))
        z.append(float(row[2]))

      err = []
      err1 = []
      err2 = []
      for i in range(len(x)):
        if errk>0:
          e = errk*math.sqrt(y[i]*(1-y[i])/z[i])
        else:
          e = 0
        err.append(e)
        err1.append(y[i]+e)
        err2.append(y[i]-e)

      lbl = f['label'] + ' (%.1f%%)' % (y[0]*100)
      if len(jd['data']) == 1:
        col = cm.spectral(0)
      else:
        col = cm.spectral(c*0.9/(len(jd['data'])-1),1)
      c+=1
      # p = plt.plot(x, z, label = lbl)
      p = plt.plot(x, y, label = lbl, color = col)
      # col = p[0].get_color()
      if errk>0:
        plt.fill_between(x, err1, err2, alpha = 0.2, color = col)
        # plt.errorbar(x, z, yerr = err, fmt='.', color = col)

      td = []
      xi = 0
      for i in table[0]:
        while i > x[xi]:
          xi += 1
        td.append(y[xi])
      table.append(td)
      tablelabels.append(lbl)

  plt.legend(loc=4)

  sys.stdout.write('# ' + jd['title'] + '\n')

  tt = len(tablelabels)
  for i in range(tt):
    sys.stdout.write('# ' + ('     | ')*i + ' '*5 + '+' + ('-'*7)*(tt-i-1) + '- %d: ' % i + tablelabels[i] +'\n')
  sys.stdout.write('#' + (' |----|')*tt + '\n')

  for i in range(len(table[0])):
    sys.stdout.write(' ')
    for j in range(len(table)):
      if j == 0:
        sys.stdout.write(' %6d' % table[j][i])
      else:
        sys.stdout.write(' %6.3f' % table[j][i])
    sys.stdout.write('\n')

elif plottype == 'stages-mp': # stages of move prediction
  fig = plt.figure()
  fig.canvas.set_window_title(jd['title']) 
  plt.title(jd['title'])
  plt.xlabel('Game Stage Center')
  plt.ylabel('Move Prediction Accuracy')
  plt.grid(True)
  # plt.ylim(0,1)
  xmin = 0
  xmax = 300
  stage = 30
  if 'xmax' in jd.keys():
    xmax = int(jd['xmax'])
  if 'stage' in jd.keys():
    stage = int(jd['stage'])
  plt.xlim(xmin,xmax)
  # plt.yticks(np.append(np.arange(0,1,0.02),1))
  plt.xticks(np.append(np.arange(xmin,xmax,stage),xmax))
  errk = 0.0
  if 'errk' in jd.keys():
    errk = float(jd['errk'])

  table = []
  # table.append(range(xmin,xmax+1))
  table.append(range(xmin+stage/2,xmax+1,stage))
  tablelabels = []
  tablelabels.append('Center of game stage')

  c = 0
  for f in jd['data']:
    with open(f['file'], 'rb') as csvfile:
      x = []
      y = []
      z = []
      reader = csv.reader(csvfile)
      for row in reader:
        r0 = float(row[0])
        r1 = float(row[1])
        r2 = float(row[2])
        r3 = float(row[3])
        r4 = float(row[4])
        r5 = float(row[5])
        x.append((r0+r1)/2)
        y.append(r2)
        z.append(r4)

      err = []
      err1 = []
      err2 = []
      for i in range(len(x)):
        if errk>0:
          e = errk*math.sqrt(y[i]*(1-y[i])/z[i])
        else:
          e = 0
        err.append(e)
        err1.append(y[i]+e)
        err2.append(y[i]-e)

      lbl = f['label']
      if len(jd['data']) == 1:
        col = cm.spectral(0)
      else:
        col = cm.spectral(c*0.9/(len(jd['data'])-1),1)
      c+=1
      # p = plt.plot(x, z, label = lbl)
      p = plt.plot(x, y, label = lbl, color = col)
      # col = p[0].get_color()
      if errk>0:
        plt.fill_between(x, err1, err2, alpha = 0.2, color = col)
        # plt.errorbar(x, z, yerr = err, fmt='.', color = col)

      td = []
      xi = 0
      for i in table[0]:
        while i > x[xi]:
          xi += 1
        td.append(y[xi])
      table.append(td)
      tablelabels.append(lbl)

  plt.legend(loc=4)

  sys.stdout.write('# ' + jd['title'] + '\n')

  tt = len(tablelabels)
  for i in range(tt):
    sys.stdout.write('# ' + ('     | ')*i + ' '*5 + '+' + ('-'*7)*(tt-i-1) + '- %d: ' % i + tablelabels[i] +'\n')
  sys.stdout.write('#' + (' |----|')*tt + '\n')

  for i in range(len(table[0])):
    sys.stdout.write(' ')
    for j in range(len(table)):
      if j == 0:
        sys.stdout.write(' %6d' % table[j][i])
      else:
        sys.stdout.write(' %6.3f' % table[j][i])
    sys.stdout.write('\n')

elif plottype == 'stages-le': # stages of mean log-evidence
  fig = plt.figure()
  fig.canvas.set_window_title(jd['title']) 
  plt.title(jd['title'])
  plt.xlabel('Game Stage Center')
  plt.ylabel('Mean Log-evidence')
  plt.grid(True)
  # plt.ylim(0,1)
  xmin = 0
  xmax = 300
  stage = 30
  if 'xmax' in jd.keys():
    xmax = int(jd['xmax'])
  if 'stage' in jd.keys():
    stage = int(jd['stage'])
  plt.xlim(xmin,xmax)
  # plt.yticks(np.append(np.arange(0,1,0.02),1))
  plt.xticks(np.append(np.arange(xmin,xmax,stage),xmax))
  errk = 0.0
  if 'errk' in jd.keys():
    errk = float(jd['errk'])

  c = 0
  for f in jd['data']:
    with open(f['file'], 'rb') as csvfile:
      x = []
      y = []
      z = []
      reader = csv.reader(csvfile)
      for row in reader:
        r0 = float(row[0])
        r1 = float(row[1])
        r2 = float(row[2])
        r3 = float(row[3])
        r4 = float(row[4])
        r5 = float(row[5])
        x.append((r0+r1)/2)
        y.append(r3)
        z.append(r5)

      err = []
      err1 = []
      err2 = []
      for i in range(len(x)):
        if errk>0:
          e = errk*math.sqrt(z[i])
        else:
          e = 0
        err.append(e)
        err1.append(y[i]+e)
        err2.append(y[i]-e)

      lbl = f['label']
      if len(jd['data']) == 1:
        col = cm.spectral(0)
      else:
        col = cm.spectral(c*0.9/(len(jd['data'])-1),1)
      c+=1
      # p = plt.plot(x, z, label = lbl)
      p = plt.plot(x, y, label = lbl, color = col)
      # col = p[0].get_color()
      if errk>0:
        plt.fill_between(x, err1, err2, alpha = 0.2, color = col)
        # plt.errorbar(x, z, yerr = err, fmt='.', color = col)

  plt.legend(loc=4)

elif plottype == '3d': # legacy 3d mp plot
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
  surf = ax.plot_surface(X, Y, Z, rstride=1, cstride=1, cmap=cm.jet)
  fig.colorbar(surf, shrink=0.5, aspect=5)

  ax.set_zlim(0,1)
  ax.set_ylim(0,xmax)

else: # legacy 2d mp plot
  fig = plt.figure()
  fig.canvas.set_window_title(jd['title']) 
  plt.title(jd['title'])
  plt.xlabel('Move Rank')
  plt.ylabel('Cumulative Probability')
  plt.grid(True)
  plt.ylim(0,1)
  xmin = 1
  xmax = 50
  if 'xmax' in jd.keys():
    xmax = int(jd['xmax'])
  plt.xlim(xmin,xmax)
  plt.yticks(np.append(np.arange(0,1,0.05),1))
  plt.xticks(np.append(np.arange(xmin,xmax),xmax))
  errk = 0.0
  if 'errk' in jd.keys():
    errk = float(jd['errk'])

  table = []
  table.append(range(xmin,xmax+1))
  tablelabels = []
  tablelabels.append('Move Rank')

  c = 0
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
      err = []
      err1 = []
      err2 = []
      for yy in y:
        t += yy
        v = t/s
        z.append(v)
        if errk>0:
          e = errk*math.sqrt(v*(1-v)/s)
        else:
          e = 0
        err.append(e)
        err1.append(v+e)
        err2.append(v-e)

      lbl = f['label'] + ' (%.1f%%)' % (z[0]*100)
      col = cm.spectral(c*0.9/(len(jd['data'])-1),1)
      c+=1
      # p = plt.plot(x, z, label = lbl)
      p = plt.plot(x, z, label = lbl, color = col)
      # col = p[0].get_color()
      if errk>0:
        plt.fill_between(x, err1, err2, alpha = 0.2, color = col)
        # plt.errorbar(x, z, yerr = err, fmt='.', color = col)

      td = []
      xi = 0
      for i in table[0]:
        while i > x[xi]:
          xi += 1
        td.append(z[xi])
      table.append(td)
      tablelabels.append(lbl)

  plt.legend(loc=4)

  sys.stdout.write('# ' + jd['title'] + '\n')

  tt = len(tablelabels)
  for i in range(tt):
    sys.stdout.write('# ' + ('     | ')*i + ' '*5 + '+' + ('-'*7)*(tt-i-1) + '- %d: ' % i + tablelabels[i] +'\n')
  sys.stdout.write('#' + (' |----|')*tt + '\n')

  for i in range(len(table[0])):
    sys.stdout.write(' ')
    for j in range(len(table)):
      if j == 0:
        sys.stdout.write(' %6d' % table[j][i])
      else:
        sys.stdout.write(' %6.3f' % table[j][i])
    sys.stdout.write('\n')

plt.show()

