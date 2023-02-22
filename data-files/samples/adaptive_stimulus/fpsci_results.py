import sqlite3 # Used for db queries

# Utility methods
def runQuery(db, query):
    # Handle option for db as filename instead of sqlite3.Connection
    if type(db) is str: db = sqlite3.connect(db)
    c = db.cursor(); c.execute(query)
    return c.fetchall()

def unpack_results(rows):
    if len(rows) > 0 and len(rows[0]) == 1:
        # Make single-item rows a single array of values
        for i, row in enumerate(rows): rows[i] = row[0]
    return rows

def runQueryAndUnpack(db, query): return unpack_results(runQuery(db, query))

# Run a query that returns a single result (first value)
def runSingleResultQuery(db, query):
    rows = runQuery(db, query)
    if len(rows) == 0: return None
    else: return rows[0][0]

# User methods
def getLastResultsFilename(dir):
    import glob, os
    files = list(glob.glob(os.path.join(dir, '*.db')))
    if len(files) == 0: return None
    files.sort(key=lambda x: os.path.getmtime(x), reverse=True)
    return files[0]

def getLastResultsDb(path):
    return sqlite3.connect(getLastResultsFilename(path))

def getUserFromDb(db):
    if type(db) is str: return db.split('_')[-2]
    elif type(db) is sqlite3.Connection: 
        return runSingleResultQuery(db, 'SELECT subject_id FROM Users LIMIT 1')

def getCurrentTaskId(db): return runSingleResultQuery(db, 'SELECT task_id FROM Tasks ORDER BY rowid DESC LIMIT 1')

def getCurrentTaskIndex(db, taskId=None):
    q = 'SELECT task_index FROM Tasks '
    if taskId is not None: q += f'WHERE task_id is "{taskId}" '
    q += 'ORDER BY rowid DESC LIMIT 1'
    return runSingleResultQuery(db, q)

def getTrialCountByTask(db, taskId=None):
    q = 'SELECT rowid FROM Trials '
    if taskId is not None: q += f'WHERE task_id is "{taskId}"'
    rows = runQuery(db, q)
    if rows is None: return 0
    return len(rows)

def getTotalTrialCount(db):
    rows = runQuery(db, f'SELECT rowid FROM Trials')
    if rows is None: return 0
    return len(rows)

def getLastQuestionResponses(db, n=1, taskId=None, taskIdx=None): 
    # You should specify a task id in order to filter on index
    if taskId is None and taskIdx is not None: raise Exception("Task index was provided but no task id!")
    # If no task ID is specified check all records
    q = 'SELECT response FROM Questions '
    if taskId is not None: 
        q += f'WHERE task_id is "{taskId}" '
        if not (taskIdx is None): q += f'AND task_index is {taskIdx} '
    q += f'ORDER BY rowid DESC LIMIT {n}'
    if n == 1: return runSingleResultQuery(db, q)
    else: return runQueryAndUnpack(db, q)

def getLastTrialIds(db, n=1, taskId=None, taskIndex=None): 
    q = f'SELECT trial_id FROM Trials '
    if taskId is not None:
        q +=  f'WHERE task_id is "{taskId}" '
        if taskIndex is not None: q += f'AND task_index is {taskIndex} '
    q += f'ORDER BY rowid DESC LIMIT {n}'

    if n == 1: return runSingleResultQuery(db, q)
    else: return runQueryAndUnpack(db, q)

def getLastTrialParams(db, paramName, n=1, taskId=None, taskIndex=None):
    q = f'SELECT {paramName} FROM Trials '
    if taskId is not None: 
        q += f'WHERE task_id is "{taskId}" '
        if taskIndex is not None: q += f'AND task_index is {taskIndex} '
    q += f'ORDER BY rowid DESC LIMIT {n}'
    if n == 1: return runSingleResultQuery(db, q)
    else: return runQueryAndUnpack(db, q)
