import sqlite3
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os
from os import listdir
from os.path import isfile, join
import datetime



# Define the path to the folder containing all DB files
global path
path = r'C:\Users\\NVIDIA\Documents\\Python Scripts\\data\\Pilot_2\\'

IN_LOG_TIME_FORMAT = '%Y-%m-%d %H:%M:%S.%f'
convert2Time = lambda x: datetime.datetime.strptime(x, IN_LOG_TIME_FORMAT)
findClosestTime = lambda timeStamp, vector : abs(vector - timeStamp).idxmin()

def concat_all_DBs(path):
	onlyfiles = [f for f in listdir(path) if isfile(join(path, f))]
	print('The file Names:\n ', onlyfiles)
	
	conditionDF = []
	trialDF = []
	targetDF = []
	playerDF = []
	experimentDF = []
	IVPairs = []

	conditionIdx = 0
	trialIdx = 0
	targetIdx = 0
	playerIdx = 0
	experimentIdx = 0

	for fileName in onlyfiles:
	    #print(fileName)
	    dat = sqlite3.connect(path + fileName)
	    cursor = dat.cursor()
	    cursor.execute("SELECT name FROM sqlite_master WHERE type='table';")
	    condition = pd.read_sql_query("SELECT * FROM Conditions", dat)
	    if len(condition) == 0:
	        continue
	    print(fileName)
	    print(cursor.fetchall())
	    trials = pd.read_sql_query("SELECT * FROM Trials", dat)
	    trials['refresh_rate'] = condition.refresh_rate.values[0]
	    latencyMS = np.round(1000*condition.added_frame_lag.values[0]/condition.refresh_rate.values[0],0)
	    #print(condition.added_frame_lag.values[0],condition.refresh_rate.values[0], latencyMS)
	    condition['added_frame_lag'] = latencyMS
	    trials['added_frame_lag'] = latencyMS
	    target = pd.read_sql_query("SELECT * FROM Target_Trajectory", dat)
	    target['position_az'] = np.arctan2(-target.position_z.values,-target.position_x.values)* 180 / np.pi
	    target['position_el'] = np.arctan2(target.position_y.values,np.sqrt(np.power(target.position_x.values,2) + np.power(target.position_z.values,2))) * 180 / np.pi
	    target['refresh_rate'] = condition.refresh_rate.values[0]
	    target['added_frame_lag'] = latencyMS
	    player = pd.read_sql_query("SELECT * FROM Player_Action", dat)
	    player['refresh_rate'] = condition.refresh_rate.values[0]
	    player['added_frame_lag'] = latencyMS
	    experiment = pd.read_sql_query("SELECT * FROM Experiments", dat)
	    IVPairs.append([condition.refresh_rate.values[0], latencyMS])
	    
	    
	    conditionDF.append(condition)
	    trialDF.append(trials)
	    targetDF.append(target)
	    playerDF.append(player)
	    experimentDF.append(experiment)
	    
	allConditions = pd.concat(conditionDF, ignore_index=True)
	allTrials = pd.concat(trialDF, ignore_index=True)
	allTargets = pd.concat(targetDF, ignore_index=True)
	allPlayers = pd.concat(playerDF, ignore_index=True)
	allExperiments = pd.concat(experimentDF, ignore_index=True)

	return allConditions, allTrials, allTargets, allPlayers, allExperiments


def data_preprocessing(allConditions, allTrials, allTargets, allPlayers, allExperiments):

	# Convert the strings to time objects
	allPlayers['time'] = allPlayers['time'].apply(convert2Time)
	allTargets['time'] = allTargets['time'].apply(convert2Time)
	allTrials['start_time'] = allTrials['start_time'].apply(convert2Time)
	allTrials['end_time'] = allTrials['end_time'].apply(convert2Time)

	# Find the start and end index for each trial in Player data frame
	allTrials['player_global_startIdx'] = allTrials['start_time'].apply(findClosestTime, vector = allPlayers['time'])
	allTrials['player_global_endIdx'] = allTrials['end_time'].apply(findClosestTime, vector = allPlayers['time'])

	# Find the start and end index for each trial in Target data frame
	allTrials['target_global_startIdx'] = allTrials['start_time'].apply(findClosestTime, vector = allTargets['time'])
	allTrials['target_global_endIdx'] = allTrials['end_time'].apply(findClosestTime, vector = allTargets['time'])
	return allTrials

def plot_execution_time_error(table = 'player'):
	myArray = []
	for trialIndex in range(len(allTrials)):
		if (table == 'player'):
	    	myArray.append( allTrials['task_execution_time'].values[trialIndex] - (allTrials['player_global_endIdx'].values[trialIndex] - allTrials['player_global_startIdx'].values[trialIndex])/allTrials['refresh_rate'].values[trialIndex])
	    else:
	    	myArray.append( allTrials['task_execution_time'].values[trialIndex] - (allTrials['target_global_endIdx'].values[trialIndex] - allTrials['target_global_startIdx'].values[trialIndex])/allTrials['refresh_rate'].values[trialIndex])
	#print('execution time : ', allTrials['task_execution_time'].values[trialIndex])
	#print('execution time : ', (allTrials['player_global_endIdx'].values[trialIndex] - allTrials['player_global_startIdx'].values[trialIndex])/allTrials['refresh_rate'].values[trialIndex])
	#allTrials.apply(playerTimeDifference)
	#allTrials
	myArray = np.array(myArray)
	plt.figure(figsize = (10,8))
	num_bins = 50
	if (table == 'player'):
		myColor = 'blue'
	else:
		myColor = 'blue'
	# the histogram of the data
	n, bins, patches = plt.hist(myArray[abs(myArray)<0.1], num_bins, density=True, facecolor=myColor, edgecolor = 'black',alpha=0.5)
	 
	# add a 'best fit' line
	#y = mlab.normpdf(bins, mu, sigma)
	#plt.plot(bins, y, 'r--')
	plt.xlabel('time [s]', fontsize = 16)
	plt.ylabel('Probability', fontsize = 16)
	plt.title('Histogram of Execution Time Difference \n for '+table+' Data', fontsize = 16)
	plt.grid(True) 
	# Tweak spacing to prevent clipping of ylabel
	#plt.subplots_adjust(left=0.15)
	plt.show()

def findIndexInPlayerDF(allClickLatency, allPlayers):
    myArray = []
    for idx in range(len(allClickLatency)):
        timeStamp = allClickLatency['time'].values[idx]
        myArray.append(np.argmin(abs(allPlayers['time'].values - timeStamp)))
    allClickLatency['player_global_Idx'] = np.array(myArray)


if __name__ == '__main__':
	allConditions, allTrials, allTargets, allPlayers, allExperiments = concat_all_DBs(path)

	allTrials = data_preprocessing(allConditions, allTrials, allTargets, allPlayers, allExperiments)
	plot_execution_time_error(table = 'player')
	plot_execution_time_error(table = 'target')
