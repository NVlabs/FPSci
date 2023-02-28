import adaptive_stimulus.fpsci_results as results
import adaptive_stimulus.fpsci_task_gen as taskgen
import numpy as np

# File paths
RESULTS_DIR = './results/'  # Path to current results directory
CONFIG_DIR = './'           # Path to output trials.Any file
OUTPUT_LOG = 'pylog.txt'    # Path to write log of runtime to (for debug)

# Adaptation constants
BASE_FRAMERATE_HZ = 60      # The base frame rate to compare to
INITIAL_STEP_PERCENT = 80   # The amount of the base frame rate to step up/down (negative to step down)

# Stop conditions
ND_TO_END = 4               # The amount of answers of "these are the same condition" to terminate
MAX_QUESTIONS = 100         # An alternate end crtiera (must be > ND_TO_END!)

# The (single) question configuration for our experiment (could be a list)
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
f = open(OUTPUT_LOG, 'a+')  # Open in append mode (keep previous results)
sys.stdout = f

# Trap case for when configuration above is incorrect
if MAX_QUESTIONS < ND_TO_END: 
    print("ERROR: Must have ND_END < MAX_QUESTIONS!")
    exit()

# Get results file(name) and print it to console
from datetime import datetime
fname = results.getLastResultsFilename(RESULTS_DIR)
print(f'\nReading {fname} @ {datetime.now()}...')

# Open results database
db = results.getLastResultsDb(RESULTS_DIR)

# 2 ways to get the task id (hard code or query database)
TASK_ID = 'fr_adapt' # Hard-coding this requires keeping it in-sync with the config file, but avoids edge-cases
# TASK_ID = results.getCurrentTaskId(db)  # (Alternative) read whatever the most recent task_id is from the experiment config file (should be written by FPSci)
# if TASK_ID is None: print('WARNING: Did not find a valid task_id in the database!')

TASK_INDEX = results.getCurrentTaskIndex(db, TASK_ID)
if TASK_INDEX is None: 
    print(f'WARNING: Did not find a valid task_index for task {TASK_ID}, using 0 instead!')
    TASK_INDEX = 0
print(f'Using task id: "{TASK_ID}" and index: {TASK_INDEX}')

# Print trial status (informational only)
total_trials = results.getTotalTrialCount(db)
task_trial_cnt = results.getTrialCountByTask(db, TASK_ID)
idx_trial_cnt = results.getTrialCountByTask(db, TASK_ID, TASK_INDEX)
print(f'Results database contains a total of {total_trials} trial(s). {task_trial_cnt} ({idx_trial_cnt}) for this task (index)!')

# Use time to filter out only questions that come from our current task iteration
START_TIME = results.getTaskStartTime(db, TASK_ID, TASK_INDEX)
print(f'Filtering responses using task start time of: {START_TIME}')

# Get answers for questions from our task (used for generating next task)
answers = results.getLastQuestionResponses(db, MAX_QUESTIONS, TASK_ID, TASK_INDEX, START_TIME)
print(f'Got {len(answers)} responses from results file...')

# Check various termination conditions
done = False  # Flag indicating the task is complete
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
        print(f"No responses from task {TASK_ID} creating first set...")
        otherRate = BASE_FRAMERATE_HZ * (1.0+INITIAL_STEP_PERCENT/100.0)
        rates = [BASE_FRAMERATE_HZ, otherRate]
    else: # At least 1 previous trial has been run
        # Try to get last 2 rates from the
        try: lastRates = results.getLastTrialParams(db, 'frameRate', 2, TASK_ID, TASK_INDEX) # Get last 2 frame rates from trials table
        except: 
            print('ERROR: Could not get "frameRate" parameter from previous trials, make sure it is in your experiment/session trialParamsToLog!')
            exit() # Cannot recover from this condition (don't know previous frame rates)...
        lastRates = [float(r) for r in lastRates]  # Convert to float
        print(f"Found last trial rates of {lastRates[0]} and {lastRates[1]} Hz")
        
        #TODO: Replace this logic in the future (code decides how many answers were "yes I can tell a difference" required to adapt)
        answerCount = min(len(answers), ND_TO_END)
        ndRatio = sum([1 for a in answers[:answerCount] if 'no' in a.lower()]) / answerCount
        print(f'{100*ndRatio}% of last {answerCount} responses were "no difference"...')

        if ndRatio >= 0.5: # Step away from the baseline
            print('Mostly "no difference": keep last levels')
            rates = lastRates
        else: # Step towards the baseline
            print('Mostly report a difference: move towards baseline')
            rates = [np.average(lastRates), BASE_FRAMERATE_HZ]

        print(f'Next rates will be {rates}Hz...')
    
    # Generate task (w/ shuffled condition order)
    import random; random.shuffle(rates)
    config = taskgen.singleParamTaskConfig('frameRate', rates, TARGETS, QUESTION, correctAnswer='Yes')   

# Write task config to trials.Any file
taskgen.writeToAny(CONFIG_DIR, config)
print('Wrote config to trials.Any!')

# Restore original stdout 
sys.stdout = orig_stdout
f.close()
