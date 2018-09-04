import os
import sys
import numpy as np

start_time = int (sys.argv[1])
end_time = int (sys.argv[2])
interval = float (sys.argv[3])

sender_node_id = range (3, 43)
receiver_node_id = range (43, 64)

if (not os.path.isdir("../results/alpha_data")):
  os.system("mkdir ../results/alpha_data")

for i in sender_node_id:
  os.system ("cat ../files-"+str(i)+"/var/log/*/stdout > "+"../results/alpha_data/"+str(i))

for i in sender_node_id:
  time = np.arange (float (start_time), float (end_time), interval)
  alpha_data = []
  with open ("../results/alpha_data/"+str(i)) as f:
    for line in f:
      j = 0
      m = line.find ("alpha")
      if (m == -1):
        continue
      for k in range (m, len (line)):
         if (ord(line[k]) >= 48 and ord (line[k]) <= 57):
           j = k
           break
      alpha = ""
      for o in range (j, len (line)):
        if (ord (line[o]) < 48 or ord (line[o]) > 58):
          break;
        alpha+=str (line [o])
      alpha_data.append (str(float(alpha)/float(1024)))
  f.close ()
  format_data_file = open ("../results/alpha_data/"+str(i)+"_format.txt", "w+")
  print_val = min (len (alpha_data), len (time))
  pop_val = len (time) - print_val
  for k in range(pop_val):
    time = np.delete (time, 0)
  for z in range(len (alpha_data)):
    format_data_file.write (str(time[z])+","+alpha_data[z]+"\n")
  
