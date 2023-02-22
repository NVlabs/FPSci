import adaptive_stimulus.fpsci_results as results
import adaptive_stimulus.fpsci_task_gen as taskgen
import numpy as np

RESULTS_DIR = './results/'   # Path to current results directory
CONFIG_DIR = './'            # Path to output trials.Any file
OUTPUT_LOG = 'pylog.txt'   # Path to write log of runtime to (for debug)

# Adaptation constants
BASE_FRAMERATE_HZ = 60 # The base frame rate to compare to
INITIAL_STEP_PERCENT = 80 # The amount of the base frame rate to step up/down (negative to step down)

ND_TO_END = 4 # The amount of answers of "these are the same condition" to terminate
MAX_QUESTIONS = 100 # An alternate end crtiera (must be > ND_TO_END!)
if MAX_QUESTIONS < ND_TO_END: raise "ERROR: Must have ND_END < MAX_QUESTIONS!"

# This is the question for our experiment
QUESTION = [
    {
        'type': 'MultipleChoice',
        'prompt': 'Was this trial different than the previous one?', 
        'options': ['yes', 'no'],
        'optionKeys': ['y', 'n'],
    }
]
# Targets to use for all trials (referenced from experiment config)
TARGETS = ["moving", "moving", "moving"]

# Redirect print output to log file
import sys
orig_stdout = sys.stdout
f = open(OUTPUT_LOG, 'a+') # Open in append mode (keep previous results)
sys.stdout = f

# Get results file(name) and print it to console
from datetime import datetime
fname = results.getLastResultsFilename(RESULTS_DIR)
print(f'\nReading {fname} @ {datetime.now()}...')
db = results.getLastResultsDb(RESULTS_DIR)

TASK_ID = results.getCurrentTaskId(db) # This comes from the experiment config file (keep up to date)
TASK_INDEX = results.getCurrentTaskIndex(db, TASK_ID)
print(f'Using task id: "{TASK_ID}" and index: {TASK_INDEX}')

# Print trial status (informational)
taskTrialCount = results.getTrialCountByTask(db, TASK_ID)
print(f'Db contains a total of {results.getTotalTrialCount(db)} trial(s) ({taskTrialCount} for this task)')

done = False  # Flag indicating the task is complete

# Get answers for questions from our task
answers = results.getLastQuestionResponses(db, MAX_QUESTIONS, TASK_ID, TASK_INDEX)
print(f'Got {len(answers)} responses from results file...')

# Check various termination/error conditions
if(len(answers) >= MAX_QUESTIONS): 
    # We have asked too many questions (forced end w/o convergence)
    print('Termination criteria reached: maximum question count!')
    done = True 
elif len(answers) >= ND_TO_END: 
    # Check for enough "no difference" responses to end
    done = True    
    for a in answers[:ND_TO_END]: 
        # If any answer is "yes there is a difference" we are not done
        if 'yes' in a.lower():  done = False; break
    if done: print(f'Termination criteria reached: {ND_TO_END} consectuive "no difference" responses!')

# Create an empty task config (signals end of adaptation if unpopulated)
config = taskgen.emptyTaskConfig() 

# Update the next set of trials based on previous responses
if not done:
    if(len(answers) == 0): # First time running this task (create initial conditions)
        print(f"No responses from task {TASK_ID} creating first set!")
        otherRate = BASE_FRAMERATE_HZ * (1.0+INITIAL_STEP_PERCENT/100.0)
        rates = [BASE_FRAMERATE_HZ, otherRate]
    else: # At least 1 previous trial has been run
        # Try to get last 2 rates from the
        try: lastRates = results.getLastTrialParams(db, 'frameRate', 2, TASK_ID, TASK_INDEX) # Get last 2 frame rates from trials table
        except: 
            print('ERROR: Could not get "frameRate" parameter from previous trials, make sure it is in trialParamsToLog!')
            exit() # Cannot recover from this condition...
        lastRates = [float(r) for r in lastRates]  # Convert to float
        print(f"Found last trial rates of {lastRates[0]} and {lastRates[1]} Hz")
        
        #TODO: Replace this logic in the future (code decides how many answers were "yes I can tell a difference" required to adapt)
        ndRatio = sum([1 for a in answers[:ND_TO_END] if 'no' in a.lower()]) / ND_TO_END
        print(f'{100*ndRatio}% of last {ND_TO_END} responses were "no difference"...')

        if ndRatio >= 0.5: # Step away from the baseline
            print('Mostly "no difference": keep last levels')
            rates = lastRates
        else: # Step towards the baseline
            print('Mostly report a difference: move towards baseline')
            rates = [np.average(lastRates), BASE_FRAMERATE_HZ]
        print(f'Next rates will be {rates}Hz...')
    
    # Generate task (w/ shuffled condition order)
    import random
    random.shuffle(rates)
    config = taskgen.singleParamTaskConfig('frameRate', rates, TARGETS, QUESTION, correctAnswer='Yes')   

# Write task config to trials.Any file
taskgen.writeToAny(CONFIG_DIR, config)
print('Wrote config to trials.Any!')

# Restore original stdout 
sys.stdout = orig_stdout
f.close()
