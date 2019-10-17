import sqlite3
import math
from datetime import datetime

IN_LOG_TIME_FORMAT = '%Y-%m-%d %H:%M:%S.%f'

class Trial:
    def __init__(self, conditionId, sessName, sessMode, startTime, endTime, taskExecTime, success, index=-1):
        self.id = conditionId
        self.idx = index
        self.sess = sessName
        self.mode = sessMode
        self.startTime = startTime
        self.endTime = endTime
        self.taskExecTime = taskExecTime
        self.success = bool(success)

class PlayerAction:
    def __init__(self, t, pos_az, pos_el, pos_x, pos_y, pos_z, event, targetId=None):
        self.time = datetime.strptime(t, IN_LOG_TIME_FORMAT)
        self.view_az = float(pos_az)
        self.view_el = float(pos_el)
        self.pos_x = float(pos_x)
        self.pos_y = float(pos_y)
        self.pos_z = float(pos_z)
        self.event = event
        self.targetId = targetId

class Target:
    def __init__(self, targetId, trialId, tType, tParams, model):
        self.id = targetId
        self.trial = trialId
        self.type = tType
        self.params = tParams
        self.model = model

class QuestionResponse:
    def __init__(self, sessionId, question, response):
        self.sessId = sessionId
        self.question = question
        self.response = response

class Event:
    def __init__(self, time, eventType):
        self.time = time
        self.type = eventType

class Click:
    def __init__(self, time, azim, elev, hit, clicktophoton):
        self.time = time
        self.azim = azim
        self.elev = elev
        self.hit = hit
        self.clicktophoton = clicktophoton

class Importer:
    """Simple class for importing data from abstract-fps results files"""

    def __init__(self, dbName):
        self.db = sqlite3.connect(dbName)

    ######################################################
    # Generic Functions for DB operations
    ######################################################
    def queryDb(self, query):
        """Simple method to query the db"""
        c = self.db.cursor()
        c.execute(query)
        return c.fetchall()

    def getTableRows(self, tableName):
        """Get all rows from a particular table"""
        return self.queryDb('SELECT * FROM {0}'.format(tableName))

    ######################################################
    # FPSci Specific Tools
    ######################################################
    def getTrials(self):
        """Get all trials from the trials table"""
        trials = []
        tidx = {}
        for row in self.getTableRows('Trials'):
            if row[0] in tidx.keys(): tidx[row[0]] += 1
            else: tidx[row[0]] = 0
            trials.append(Trial(row[0], row[1], row[2], row[3], row[4], row[5], row[6], tidx[row[0]]))
        return trials

    def getCondIds(self):
        """Get a list of trial ids from the trials table"""
        ids = []
        for trial in self.getTrials(): ids.append(trial.id)
        return ids

    def getTrialsById(self, condId):
        """Get a particular trial(s) from the trials table"""
        rows = self.queryDb('SELECT * FROM Trials WHERE [condition_ID] = {0}'.format(condId))
        if(len(rows) > 1):
            out = []
            for row in rows: out.append(Trial(row[0], row[1], row[2], row[3], row[4], row[5], row[6]))
            return out
        elif(len(rows) == 1): 
            row = rows[0]
            return Trial(row[0], row[1], row[2], row[3], row[4], row[5], row[6])
        return None

    def getEvents(self):
        """Get all events from the events table"""
        events = []
        for row in self.getTableRows('event_log'): events.append(Event(row[0], row[1]))
        return events

    def getTrialTargetPositionsXYZ(self, trial, targetId=None):
        query = "SELECT * FROM Target_Trajectory WHERE [time] <= \'{0}\' AND [time] >= \'{1}\'".format(trial.endTime, trial.startTime)
        if targetId is not None: query += ' AND [target_id] = \'{2}\''.format(targetId)
        positions = {}
        for row in self.queryDb(query): 
            if row[1] not in positions.keys(): positions[row[1]] = [[row[2], row[3], row[4]]]
            else: positions[row[1]].append([row[2], row[3], row[4]])
        return positions

    def getTargetPositionsXYZ(self, condId, trialIdx=0):
        """Get all target positions (for a given condition id) from the Target_Trajectory table"""
        trial = self.getTrialsById(condId)
        if type(trial) is list: trial = trial[trialIdx]
        return self.getTrialTargetPositionsXYZ(trial)

    # Helper function for convering lists of XYZ to azim/elev
    def toAzimElev(self, target_xyz):
        """Helper for converting lists of XYZ coordinates to polar"""
        positions = {}
        for targetId in target_xyz.keys():
            positions[targetId] = []
            for [x,y,z] in target_xyz[targetId]:
                r = math.sqrt(x**2+y**2+z**2)
                azim = math.atan(z/x)
                elev = math.asin(y/r)
                positions[targetId].append([r, azim, elev])
        return positions

    def getTrialTargetPositionsAzimElev(self, trial):
        positionsXYZ = self.getTrialTargetPositionsXYZ(trial)
        return self.toAzimElev(positionsXYZ)

    def getTargetPositionsAzimElev(self, condId, trialIdx=0):
        """Get all target positions (for a given condition id) as azim/elev from Target_Trajectory table"""
        positionsXYZ = self.getTargetPositionsXYZ(condId, trialIdx)
        return self.toAzimElev(positionsXYZ)
    
    def getTrialPlayerActions(self, trial):
        """Get all player actions from a particular trial"""
        actions = []
        for row in self.queryDb("SELECT * FROM Player_Action WHERE [time] <= \'" + trial.endTime + "\' AND [time] >= \'" + trial.startTime + "\'"): 
            actions.append(PlayerAction(row[0], row[1], row[2], row[3], row[4] , row[5], row[6], row[7]))
        return actions

    def getPlayerActions(self, condId, trialIdx=0):
        """Get player actions (for a given condition id) from the Player_Action table"""
        trial = self.getTrialsById(condId)
        if type(trial) is list: trial = trial[trialIdx]
        return self.getTrialPlayerActions(trial)

    def getClicks(self):
        """Get click information from the database"""
        clicks = []
        for [t,event,azim,elev] in self.queryDb('SELECT * from Player_Action WHERE [event] = \'hit\' OR [event] = \'miss\''):
            query = 'SELECT latency from click_latencies WHERE time >= \'{0}\' ORDER BY time ASC'.format(t)
            c2p = self.queryDb(query)
            if len(c2p) == 0: c2ptime = None
            else: c2ptime = c2p[0][0]
            clicks.append(Click(t, azim, elev, event == 'hit', c2ptime))
        return clicks

    def getRowTarget(self, row):
        """Get a target from a row returned from the database"""
        # Dealing with a parametric target, get params
        if(row[2] == 'parametrized'): 
            params = {}
            params["refresh_rate"] = row[3]
            params["added_frame_lag"] = row[4]
            params["min_ecc_h"] = row[5]
            params["min_ecc_v"] = row[6]
            params["max_ecc_h"] = row[7]
            params["max_ecc_v"] = row[8]
            params["min_speed"] = row[9]
            params["max_speed"] = row[10]
            params["min_motion_change_period"] = row[11]
            params["max_motion_change_period"] = row[12]
            params["jump_enabled"] = row[13]       
        return Target(row[0], row[1], row[2], params, row[14])

    def getTarget(self, targetId):
        """Get a single target by its (unqiue) id"""
        row = self.queryDb('SELECT * from Targets WHERE target_id = \'{0}\''.format(targetId))
        if len(row) == 0: return None
        return self.getRowTarget(row[0])

    def getTrialTargets(self, trialId = None):
        """Get a list of targets based on a trial ID"""
        query = 'SELECT * from Targets '
        if trialId is not None: query += 'WHERE trial_id = \'{0}\''.format(trialId)
        rows = self.queryDb(query)
        targets = []
        for row in rows: targets.append(self.getRowTarget(row))

    def getQuestionResponses(self, sessId = None):
        query = 'SELECT * from Questions '
        if sessId is not None: query += 'WHERE Session = \'{0}\''
        rows = self.queryDb(query)
        questions = []
        for row in rows: questions.append(QuestionResponse(row[0], row[1], row[2]))
        return questions