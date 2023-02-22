VERBOSE = True # This sets whether additional checks are performed and warnings/errors printed

# An empty task (task complete signal) with optional progress value
def emptyTaskConfig(progress=None):
    config = {
        'trials': [],
    }
    if progress is not None: config['progress'] = progress
    return config

# A task which sets a single parameter to an array of values and asks a question
def singleParamTaskConfig(param, values, targetIds, questions, progress=None, questionIdx=None, correctAnswer=None, description=None):
    # Build base config
    config = {
        'trials': singleParamTrialArray(param, values, targetIds),
        'questions': questions,
    }
    # Add optional fields
    if progress is not None: config['progress'] = progress
    if description is not None: config['description'] = description
    if questionIdx is not None: config['questionIndex'] = questionIdx
    if correctAnswer is not None: config['correctAnswer'] = correctAnswer
    return config

# Note this method returns a trials array, not a complete task config!
def singleParamTrialArray(param, values, targetIds, randomizeOrder=False):
    if VERBOSE:
        if type(targetIds) is not list: print(f'ERROR: Provided targetIds must be a list (is a {type(targetIds)})!')
        if len(values) == 0: print('ERROR: No values provided!')
    trials = []
    if randomizeOrder: 
        import random
        random.shuffle(values)
    for val in values:
        # Create a trial
        trials.append({
            'id': f'{val}',
            f'{param}': val,
            'targetIds': targetIds  # This assumes identical targets for all trials
        })
    return trials

# Set the target ids for a specific trial (allows customization of targets on a per-trial basis)
def setTrialTargetIds(config, trialId, targetIds):
    if type(targetIds) is not list and VERBOSE: 
        raise f'Provided targetIds must be a list (is a {type(targetIds)})!'
    # Find the trial in the config array and modify its targetIds field
    for t in config['trials']:
        if t['id'] == trialId: 
            t['targetIds'] = targetIds # Update the targetIds
            return
    raise f'Could not find trial with id {trialId}!'

# Write this configuration to an any file (defaults to trials.Any)
def writeToAny(path, config, fname='trials.Any'):
    import os, json
    # Add filename if missing from provided path
    if os.path.isdir(path): path = os.path.join(path, fname)
    if not path.endswith('Any') and VERBOSE: print('WARNING: Results should be written to a ".Any" file!')
    # Dump JSON to file
    with open(path, 'w') as f: json.dump(config, f, indent=4)

