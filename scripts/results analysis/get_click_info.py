import sys
from FPSci_.Importer import Importer

if len(sys.argv) < 2: raise Exception("Provide input db as argument!")
db = Importer(sys.argv[1])
clicks = db.getClicks()

for click in clicks: 
    if click is None: continue
    print("Click at time t = {0}: azim = {1}°, elev = {2}°, hit = {3}, click-to-phton = {4} ms".format(click.time, click.azim, click.elev, click.hit, click.clicktophoton))