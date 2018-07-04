import os
import sys
import numpy as np

start_time = int (sys.argv[1])
end_time = int (sys.argv[2])
interval = float (sys.argv[3])

sender_node_id = range (3, 43)
receiver_node_id = range (43, 64)

if (not os.path.isdir("../results/cwnd_data")):
  os.system("mkdir ../results/cwnd_data")

for i in sender_node_id:
  os.system ("cat ../files-"+str(i)+"/var/log/*/stdout > "+"../ns-3-dce/results/cwnd_data/"+str(i))

time = np.arange (float (start_time), float (end_time), interval)

for i in sender_node_id:
  cwnd_data = []
  with open ("../ns-3-dce/results/cwnd_data/"+str(i)) as f:
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
  format_data_file = open ("../results/cwnd_data/"+str(i)+"_format.txt", "w+")
  print_val = min (len (cwnd_data), len (time))
  for k in range(print_val):
    format_data_file.write (str(time[k])+","+cwnd_data[k]+"\n")
  
