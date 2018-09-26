import os
import sys
import numpy as np

'''
Take time as command line 
argument to map with cwnd values.
'''
start_time = int (sys.argv[1])
end_time = int (sys.argv[2])
interval = float (sys.argv[3])

'''
Set the range for sender nodes id and 
receiver node id based on id of files 
folder generated on running the simulation.
'''
sender_node_id = range (2, 7)
receiver_node_id = range (7, 12)

if (not os.path.isdir("../results/gfc-dumbbell/cwnd_data")):
  os.system("mkdir ../results/gfc-dumbbell/cwnd_data")

'''
Collect the data from files folder corresponding 
to each sender node and stores in a file.
'''
for i in sender_node_id:
  os.system ("cat ../files-"+str(i)+"/var/log/*/stdout > "+"../results/gfc-dumbbell/cwnd_data/"+str(i))

'''
For each file 
'''
for i in sender_node_id:
  time = np.arange (float (start_time), float (end_time), interval)
  cwnd_data = []
  with open ("../results/gfc-dumbbell/cwnd_data/"+str(i)) as f:
    for line in f:
      j = 0
      m = line.find ("cwnd")
      if (m == -1):
        continue
      for k in range (m, len (line)):
         if (ord(line[k]) >= 48 and ord (line[k]) <= 57):
           j = k
           break
      cwnd = ""
      for o in range (j, len (line)):
        if (ord (line[o]) < 48 or ord (line[o]) > 58):
          break;
        cwnd+=str (line [o])
      cwnd_data.append (cwnd)
  f.close ()
  format_data_file = open ("../results/gfc-dumbbell/cwnd_data/"+str(i)+"_format.txt", "w+")
  print_val = min (len (cwnd_data), len (time))
  pop_val = len (time) - print_val
  for k in range(pop_val):
    time = np.delete (time, 0)
  for z in range(len (cwnd_data)):
    format_data_file.write (str(time[z])+","+cwnd_data[z]+"\n")

