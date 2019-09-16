import sys
import sqlite3
from datetime import datetime

IN_LOG_TIME_FORMAT = '%Y-%m-%d %H:%M:%S.%f'

if len(sys.argv) < 2:  raise Exception("Provide filename as input!")

outfile = 'out.Any'
if len(sys.argv) > 2: outfile = sys.argv[2]


# Open the database file provided as input
infile = sys.argv[1]
db = sqlite3.connect(infile)

# Get the player positions from the Player Action table
query = 'SELECT time, position_x, position_y, position_z FROM Player_Action'
c = db.cursor()
c.execute(query)
rows = c.fetchall()

# Extract time stamps and xyz coordinates
t = []
xyz = []
t0 = datetime.strptime(rows[10][0], IN_LOG_TIME_FORMAT)
for row in rows[10:]:
    t.append((datetime.strptime(row[0], IN_LOG_TIME_FORMAT)-t0).total_seconds())
    xyz.append([row[1], row[2], row[3]])

# Print output
output = open(outfile, 'w')
output.write("\"destSpace\": \"world\",\n")
output.write("\"destinations\": [\n")
for i in range(len(t)):
    output.write("\t{{\"t\":{0:0.6f}, \"xyz\":Point3({1:0.6f},{2:0.6f},{3:0.6f})}},\n".format(t[i], xyz[i][0], xyz[i][1], xyz[i][2]))
output.write("],")
output.close()
print("Wrote output to {0}.".format(outfile))