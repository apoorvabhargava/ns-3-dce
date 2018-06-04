import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
import sys
import pandas as pd


outputfile = sys.argv[1]
xlabel = "time(s)"
ylabel = "cwnd"
title = ""
lTitle = "Node"

ipData0 = []
ipData1 = []
#ipData2 = []
div = 1000000000

filename = ['cwnd','time']

for index in range (2):
  path = "../"
  path += filename[index]
  with open (path) as f:
    for line in f:
      if (filename[index] == 'cwnd'):
        ipData0.append (float (line.rstrip ('\n')))
      elif (filename[index] == 'time'):
        ipData1.append (float (line.rstrip ('\n'))/div)

print len (ipData0)
print len (ipData1)
print ipData0
print ipData1
ipData1.pop ()


'''
Set labels and titles
'''
plt.xlabel (xlabel)
plt.ylabel (ylabel)
plt.title (title)


'''
Plot graph against 0th 
column to every column.
'''
plt.plot (ipData1, ipData0)

legends = ["l1"]

'''
Set legends and title 
'''
#box = ax.get_position()
#ax.set_position([box.x0, box.y0, box.width * 0.80, box.height])
#ax.legend(legends, loc='center left', title=lTitle, bbox_to_anchor=(1, 0.5))

#plt.show()

'''
Save the graph
'''

plt.savefig (outputfile) 
