import matplotlib.pyplot as plt
from abstractfps_importer.Importer import Importer
import sys
import numpy as np

AZIM_RANGE_DEG = 40         # Range of azimuths to plot
ELEV_RANGE_DEG = 40         # Range of elevations to plot

if len(sys.argv) < 2: raise Exception("Provide db filename as input!")

# Get the database
db = Importer(sys.argv[1])
trials = db.getTrials()

# Analyze each trial in the results
for trial in trials:
    # Get target trajectory and player actions from the db
    trajectory = db.getTrialTargetPositionsAzimElev(trial)
    actions = db.getTrialPlayerActions(trial)

    # Start by making a plot of the target trajectory
    tazims = []; televs = []
    for pt in trajectory:
        tazims.append(pt[1]*180.0/np.pi)
        televs.append(pt[2]*180.0/np.pi)
    plt.scatter(tazims, televs, color='lightblue', s=5, marker='s')

    # Now plot the player actions
    aim_azim = []; aim_elev = []; miss_azim = []; miss_elev = []; hit_azim = []; hit_elev = []
    for pt in actions:
        if(pt[2] == "aim"):
            aim_azim.append(pt[0])
            aim_elev.append(pt[1])
        elif (pt[2] == "miss"):
            miss_azim.append(pt[0])
            miss_elev.append(pt[1])
        elif (pt[2] == "hit"):
            hit_azim.append(pt[0])
            hit_elev.append(pt[1])

    mean_azim = (np.mean(tazims) + np.mean(aim_azim))/2
    mean_elev = (np.mean(televs) + np.mean(aim_elev))/2

    # Update the plot
    plt.scatter(aim_azim, aim_elev, color='lightgray',s=1)
    plt.scatter(miss_azim, miss_elev, color='red', s=30, marker='x')
    plt.scatter(hit_azim, hit_elev, color='green', s=30, marker='o')

    plt.title('Azimuth vs Elevation for Target Path and Player Action for Trial {0} #{1}'.format(trial.id, trial.idx+1))
    plt.xlabel('Azimuth (°)')
    plt.xlim(mean_azim-AZIM_RANGE_DEG/2, mean_azim+AZIM_RANGE_DEG/2)
    plt.ylabel('Elevation (°)')
    plt.ylim(mean_elev-ELEV_RANGE_DEG/2, mean_elev+ELEV_RANGE_DEG/2)
    plt.legend(['Target Trajectory', 'Aim Trajectory', 'Miss', 'Hit'])

    plt.show()