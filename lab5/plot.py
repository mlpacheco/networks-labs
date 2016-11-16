import numpy as np
import matplotlib.pyplot as plt
import sys

f = open(sys.argv[1])
time = []; values = []
for line in f:
    (timestamp, value) = line.strip().split()
    time.append(timestamp)
    values.append(value)

time = map(float, time)
time = [round(x - time[0],2) for x in time]
values = map(int, values)

plt.figure(1)
plt.plot(time, values)
plt.show()
