#!/usr/bin/env python
#!/bin/env python
#
#    Computers speed up of Streams benchmark results generated by make streams and plots
#
#    matplotlib can switch between different backends hence this needs to be run 
#    twice to first generate a file and then display a window
#
import os
#
def process(fileoutput = 1):
  import re
  ff = open('scaling.log')
  data = ff.read()
  ff.close()

  hosts  = {}
  triads = {}
  speedups = {}
  match = data.split('Number of MPI processes ')
  for i in match:
    if i:
      fields = i.split('\n')
      size = int(fields[0])
      hosts[size] = fields[1:size+1]
      triads[size] = float(fields[size+5].split()[1])

  ff = open('scaling.log','a')
  if fileoutput: print 'np  speedup'
  if fileoutput: ff.write('np  speedup\n')
  for sizes in hosts:
    speedups[sizes] = triads[sizes]/triads[1]
    if fileoutput: print sizes,round(triads[sizes]/triads[1],2)
    if fileoutput: ff.write(str(sizes)+' '+str(round(triads[sizes]/triads[1],2))+'\n')
  ff.close()

  try:
    import matplotlib
  except:
    print "Unable to open matplotlib to plot speedup"
    return

  try:
    if fileoutput: matplotlib.use('Agg')
    import matplotlib.pyplot as plt
  except:
    print "Unable to open matplotlib to plot speedup"
    return

  try:
    plt.title('Perfect and Streams Speedup')
    plt.plot(hosts.keys(),hosts.keys(),'b-o',hosts.keys(),speedups.values(),'r-o')
    plt.show()
    if fileoutput: plt.savefig('scaling.png')
  except:
    if fileoutput: print "Unable to plot speedup to a file"
    else: print "Unable to display speedup plot"
    return

#
#
if __name__ ==  '__main__':
  import sys
  process(len(sys.argv)-1)


