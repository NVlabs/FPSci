import math
from waypoint_writer import WaypointWriter, Destination
import os, shutil

TSTEP_S = 1/360.0
ANG_OFFSET = 270 # with respect to x-axis (the 2D angular coordinate convention)

target_names = ["Cw", "CC"] # clockwise / counter-clockwise

for target_name in target_names:
	src_filename = target_name + ".Any"

	wpWriter = WaypointWriter(src_filename, target_name, [0.75, 0.75])

	# determine target trajectory
	X_SIZE = 3.5
	X_OFFSET = 0.0
	# Y_SIZE = 3.0
	# Y_OFFSET = 4.2
	Y_SIZE = 0.0
	Y_OFFSET = 1.1
	Z_OFFSET = -15.0
	ANG_SPEED_DEG_S = 80

	# determine target angular velocity
	if "Cw" in target_name:
		ANG_VEL_DEG_S = ANG_SPEED_DEG_S * -1
	else:
		ANG_VEL_DEG_S = ANG_SPEED_DEG_S

	SAMPLES = int((360/ANG_SPEED_DEG_S)/TSTEP_S)
	print("Creating {0} samples...".format(SAMPLES))

	for i in range(SAMPLES):
	    t = i * TSTEP_S
	    x = X_SIZE * math.cos((math.pi/180)*ANG_VEL_DEG_S*t + math.pi/180 * ANG_OFFSET) + X_OFFSET
	    y = Y_SIZE * math.sin((math.pi/180)*ANG_VEL_DEG_S*t + math.pi/180 * ANG_OFFSET) + Y_OFFSET
	    z = Z_OFFSET
	    print("{0}: [{1}, {2}, {3}]".format(t, x, y, z))
	    wpWriter.writeDestination(Destination(t, [x,y,z]))


	wpWriter.close()

	src = src_filename
	dst = "../../data-files/" + src_filename
	shutil.copyfile(src, dst)