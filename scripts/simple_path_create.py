import numpy as np
import random
import csv
import sys

# Output controls
PLOT_PATH = True                # Plot the resulting path

# Input controls
DISTANCE_TO_TARGET = 30.0       # Distance to the target
INITIAL_OFFSET_DEG = 15         # Initial distance from player view direction

# MAX_ECC_H_DEG = 15              # Maximum horizontal eccentricity (in deg)
# MIN_ECC_H = 0                 # Minimum horizontal eccentricity (in deg)
# MAX_ECC_V_DEG = 10              # Maximum verical eccentricity (in deg)
# MIN_ECC_V = 0                 # Minimum vertical eccentricity (in deg)

AVEL_DEGPS = 5.5                  # Constant angular velocity
# MAX_AVEL_DEGPS = 16             # Maximum (angular) velocity (in deg/s)
# MIN_AVEL_DEGPS = 7              # Minimum (angular) velocity (in deg/s)

TIME_PER_POINT = 0.5            # This is the time per point
TOTAL_TIME = 30                 # This is the total path duration

# Derrived parameters
A_DISP = TIME_PER_POINT * AVEL_DEGPS    # Angular displacement between points
POINT_COUNT = int(np.ceil(TOTAL_TIME/TIME_PER_POINT))      # Total number of points to produce

# Player position (cartesian offset) and initial view direction (polar offset)
player_position = np.array([47.5230, -2.3549, -0.3592])
view_direction =  np.zeros(3)-np.array([0.9988, -0.0478, 0.0135])

target_position = []
target_directions = []

# Convert polar to cartesian coordinates
def polar_to_cartesian(radius, azim, elev, offset=player_position):
    x = radius*np.sin(elev)*np.cos(azim)
    y = radius*np.sin(elev)*np.sin(azim)
    z = radius*np.cos(elev)
    return np.array([x,y,z])+offset

def cartesian_to_polar(x,y,z):
    r = np.sqrt(x**2+y**2+z**2)
    elev = np.arccos(z/r)
    azim = np.arctan(y/x)
    return (r,elev,azim)

def cart2pol_array(array):
    return cartesian_to_polar(array[0], array[1], array[2])

def move_target_by_angle(direction, angle_distance=A_DISP):
    [elev,azim] = direction
    mag = angle_distance*np.pi/180
    de = 2.0*(random.random()-0.5) * mag        # Pick a random component (in range) for elevation change
    da = mag * np.cos(np.arcsin(de/mag))        # Compute azimuth change so that elev+azim = angle_distance
    if random.random() < 0.5: da = 0-da
    return [elev+de, azim+da]                   # Return new azim/elev

# Get direction for target location
target_direction =  cart2pol_array(view_direction)[1:]
target_directions.append(target_direction)
target_direction = move_target_by_angle(target_direction, INITIAL_OFFSET_DEG)
target_directions.append(target_direction)
target_position.append(polar_to_cartesian(DISTANCE_TO_TARGET, target_direction[0], target_direction[1]))

# Create remaining points
for i in range(POINT_COUNT-1):
    target_direction = move_target_by_angle(target_direction)
    target_directions.append(target_direction)
    target_position.append(polar_to_cartesian(DISTANCE_TO_TARGET, target_direction[0], target_direction[1]))

# Get file output prefix
if len(sys.argv) < 2: raise Exception("Must provide file output prefix as input!")
outputFname = sys.argv[1]
outFile = open(outputFname, mode='w')
writer = csv.writer(outFile, delimiter=',', lineterminator='\n')
writer.writerow(['X','Y','Z'])

for pos in target_position:
    writer.writerow([pos[0], pos[1], pos[2]])

elev = np.swapaxes(np.array(target_directions), 0,1)[0]*180/np.pi
azim = np.swapaxes(np.array(target_directions), 0,1)[1]*180/np.pi

if PLOT_PATH:
    import matplotlib.pyplot as plt
    plt.plot(azim[1:],elev[1:], color='gray')
    plt.scatter(azim[2:],elev[2:], color='lightgreen')
    plt.scatter(azim[0], elev[0], color='red')
    plt.scatter(azim[1], elev[1], color='blue')
    plt.xlabel('Azimuth (deg)')
    plt.ylabel('Elevation (deg)')
    plt.show()