import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
import sys
import pandas as pd

sender_node_id = range (2, 7)
receiver_node_id = range (7, 12)

xlabel = "Time (sec)" 
ylabel = "Cwnd (Packets)"
title = ""
lTitle = ""
my_dpi=100

plt.figure(figsize=(800/my_dpi, 600/my_dpi), dpi=my_dpi)

for i in sender_node_id:
  '''
  Expect data file to be
  in CSV format.
  Read the data file using
  pandas library.
  '''
  inputFile= "../results/gfc-dumbbell/cwnd_data/"+str(i)+"_format.txt"
  if (i == 2):
    outputFile = "../results/gfc-dumbbell/cwnd_data/CwndA.png"
  if (i == 3):
    outputFile = "../results/gfc-dumbbell/cwnd_data/CwndB.png"
  if (i == 4):
    outputFile = "../results/gfc-dumbbell/cwnd_data/CwndC.png"
  if (i == 5):
    outputFile = "../results/gfc-dumbbell/cwnd_data/CwndD.png"
  if (i == 6):
    outputFile = "../results/gfc-dumbbell/cwnd_data/CwndE.png"

  data = pd.read_csv (inputFile, index_col=False)
  
  '''
  dict_of_lists stores each
  column as list.
  column_titles stores title
  of each column.
  '''
  dict_of_lists = []
  column_titles = []
  
  '''
  Read every column of data
  file and collect the title
  and data for each column in 
  respective list.
  '''
  for column_name in data.columns:
  	column_titles.append (column_name)
  	temp_list = data[column_name].tolist()
  	dict_of_lists.append (temp_list)
  

  if(len(column_titles) == 0):
  	sys.exit (0)

  legends = [] 
  
  '''
  Set labels and titles
  '''
  plt.xlabel (xlabel)
  plt.ylabel (ylabel)
  plt.title (title)
  plt.xticks(np.arange(12, 20, step = 1))

  ax = plt.subplot (111)
  ax.set_xlim([12,20])
  ax.set_ylim([0,200])
  ax.spines['bottom'].set_linewidth(2)
  ax.spines['left'].set_linewidth(2)
  ax.spines['top'].set_linewidth(2)
  ax.spines['right'].set_linewidth(2)
  ax.set_xticklabels(('2', '3', '4', '5', '6', '7', '8', '9', '10', '11'))
 
  '''
  Plot graph against 0th 
  column to every column.
  '''

  for k in xrange(0, len(column_titles), 2):
  	ax.plot (dict_of_lists[k], dict_of_lists[k+1], color='magenta', linewidth=3)
  
  if (i == 2):
    legends.append ("Sender-A")
  if (i == 3):
    legends.append ("Sender-B")
  if (i == 4):
    legends.append ("Sender-C")
  if (i == 5):
    legends.append ("Sender-D") 
  if (i == 6):
    legends.append ("Sender-E") 
 
  '''
  Set legends and title 
  '''
  box = ax.get_position()
  ax.set_position([box.x0, box.y0, box.width * 0.90, box.height * 1.05])
  ax.legend(legends, loc=2, title=lTitle, bbox_to_anchor=(1, 1), frameon=False)
  
  '''
  Save the graph
  '''
  plt.savefig (outputFile)
  plt.gcf().clear()
