import sqlite3
import math

class Trial:
    def __init__(self, conditionId, sessName, sessMode, startTime, endTime, taskExecTime, success):
        self.id = conditionId
        self.sess = sessName
        self.mode = sessMode
        self.startTime = startTime
        self.endTime = endTime
        self.taskExecTime = taskExecTime
        self.success = bool(success)

class Importer:
    IN_LOG_TIME_FORMAT = '%Y-%m-%d %H:%M:%S.%f'

    def __init__(self, dbName):
        self.db = sqlite3.connect(dbName)

    def getTableRows(self, tableName):
        c = self.db.cursor()
        c.execute('SELECT * FROM {0}'.format(tableName))
        return c.fetchall()

    def getTrials(self):
        c = self.db.cursor()
        c.execute('SELECT * FROM Trials')
        rows = c.fetchall()
        trials = []
        for row in rows:
            trials.append(Trial(row[0], row[1], row[2], row[3], row[4], row[5], row[6]))
        return trials

    def getTrialById(self, condId):
        trials = self.getTrials()
        for trial in trials: 
            if trial.id == condId: return trial
        return None

    def getTargetPositionsXYZ(self, condId):
        trial = self.getTrialById(condId)
        positions = []
        rows = self.db.cursor().execute("SELECT * FROM Target_Trajectory WHERE [time] <= \'" + trial.endTime + "\' AND [time] >= \'" + trial.startTime + "\'")
        for row in rows:
            positions.append([row[1], row[2], row[3]])
        return positions

    def getTargetPositionsAzimElev(self, condId):
        positionsXYZ = self.getTargetPositionsXYZ(condId)
        positions = []
        for [x,y,z] in positionsXYZ:
            r = math.sqrt(x**2+y**2+z**2)
            azim = math.atan(z/x)
            elev = math.asin(y/r)
            positions.append([r, azim, elev])
        return positions
    
    def getPlayerActions(self, condId):
        trial = self.getTrialById(condId)
        actions = []
        rows = self.db.cursor().execute("SELECT * FROM Player_Action WHERE [time] <= \'" + trial.endTime + "\' AND [time] >= \'" + trial.startTime + "\'")
        for row in rows:
            actions.append([row[2], row[3], row[1]])
        return actions