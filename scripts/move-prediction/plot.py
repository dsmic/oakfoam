#!/usr/bin/env python
# Read in a list of move prediction results and plot the results.
# The input file format is JSON (see plot_example.json).

import sys
import math
import json
import csv
import matplotlib.pyplot as plt

jd = json.loads("".join(sys.stdin.readlines()))

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

