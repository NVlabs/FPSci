import sys
import numpy as np
from FPSci_Importer.Importer import Importer
from matplotlib import pyplot as plt

# Get the database
if len(sys.argv) < 2: 
    raise Exception("Provide input db as argument!")
db = Importer(sys.argv[1])

# Get the frame info
finfo = db.getFrameInfo()

first = True
maxerror = 0
errors = []
for f in finfo:
    t = db.parseTime(f.time)
    if first: 
        first = False
        lastTime = t
        continue
    dt = float((t - lastTime).total_seconds())
    lastTime = t
    error_ms = 1e3*abs(f.sdt - dt)
    print("\t{0} --- {1:0.2f}ms vs {2:0.2f}ms --- Error = {3:0.3f}ms".format(f.time, 1e3*f.sdt, 1e3*dt, error_ms))
    errors.append(error_ms)

maxIdx = errors.index(max(errors[1:]))
print("Max error: {0}ms @ {1}\nAverage error: {2}ms".format(max(errors), maxIdx, np.average(errors)))

plt.plot(errors)
plt.xlabel("Index")
plt.ylabel("Error [ms]")
plt.show()