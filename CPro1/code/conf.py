"""Configuration parameters."""

import os
import problem_def

# LLM setup parameters
import os
API_KEY = os.getenv("API_KEY") 
if not API_KEY:
    raise ValueError("API_KEY is not set in environment variables.")

PROVIDER = os.getenv("PROVIDER")
if not PROVIDER:
    raise ValueError("PROVIDER is not set in environment variables.")
if PROVIDER not in ["OPENAI","MISTRAL","ANTHROPIC","DEEPSEEK","DEEPINFRA","OLLAMA"]:
    raise ValueError('PROVIDER must be one of: "OPENAI","MISTRAL","ANTHROPIC","DEEPSEEK","DEEPINFRA","OLLAMA"')

MODEL = os.getenv("MODEL") 
if not MODEL:
    raise ValueError("MODEL is not set in environment variables.")

SEQUENTIAL = False # False = prompt in parallel for details and candidate programs, True = prompt sequentially (e.g. for local models)
SEC_PER_REQ = 2 # limit SEQUENTIAL requests per second - pause SEC_PER_REQ seconds between initiating requests
if PROVIDER=="OLLAMA":
    SEQUENTIAL = True
    SEC_PER_REQ = 0

INITIAL_RETRY_WAIT = 10 # #seconds to wait for API retry
MAX_RETRIES = 12 # max number of retries, doubling each time.  This is enough to get through a multi-hour outage.

LANGUAGE = "C" # Language for code generation.  Currently only "C" is supported.


# Protocol parameters
BASE_SEED = 1000   
NUM_PROCESSES = 32 # Number to run in parallel.  Most efficient to set to the number of cores (or hyperthreads) supported by the hardware.

INITHYPERTUNETIME = 0.5 # number of seconds for first round hyperparameter tuning
F = 10 # each round of hyperparameter tuning, increase INITHYPERTUNETIME by a factor of F and decrease grid size by a factor of F         
GRIDSIZE = 1000 # should be a power of F.  
SHORTFLAG = False # False fo full hypertuning, True to limit hyperparmaeters to defaults.  Note that setting "hypertune" below to None will take preexisting hypertuning and ablate it as if SHORTFLAG had been True for an original run.
SEEDS = NUM_PROCESSES//len(problem_def.DEV_INSTANCES) # so that we can run SEEDS of each of DEV_INSTANCES in parallel and fit within NUM_PROCESSES
if SEEDS<=0:
    raise ValueError("problem_def.DEV_INSTANCES must be <= conf.NUM_PROCESSES")

NUM_STRATEGIES = 20 # Prompt to generate this many strategies each time.
STRATEGY_REPS = 50 # will generate NUM_STRATEGIES*STRATEGY_REPS programs
NUM_SELECT = 5 # for advancement to speed optimization and checking on all DEV_INSTANCES
OPT_TRIES = 50 # number of optimization candidates to generate per round of optimization
OPT_ROUNDS = 5 # rounds of optimization
OPT_SEEDS = SEEDS
MIN_IMPROVE = 0.1 # minimum improvement in objective score to accept an opt improvement and proceed to another round
FULL_DEV_TIME = 7200 # Time in seconds for testing NUM_SELECT final candidate on DEV_INSTANCES.
FULL_DEV_SEEDS = SEEDS
NUM_FINAL = 2  # for attempting OPEN_INSTANCES
FULL_OPEN_TIME = 172800 # Time in seconds for testing NUM_FINAL candidates on OPEN_INSTANCES
FULL_OPEN_SEEDS = NUM_PROCESSES//len(problem_def.OPEN_INSTANCES)
if FULL_OPEN_SEEDS<=0:
    raise ValueError("problem_def.OPEN_INSTANCES must be <= conf.NUM_PROCESSES")
STRAT_TEMP = 1.0 # temperature when listing strategy
DETAILS_TEMP = 1.0 # temperature when filling in strategy details
PROG_TEMP = 1.0  # temperature when creating code
TOP_P = 1.0  # constant across applications

RUN_LABEL_NUM = "1" # global label for this entire execution
INIT_RUN_NUM = 1000 # Start for sequentially labelling code files and temporary directories.

MEM_LIMIT = 3 # memory limit per process, in GB.  Integers only.  Should be less than system memory / NUM_PROCESSES.

# Set all RUN_STAGES to True for a full run.
# For restarting from a .pkl checkpoint, set initial stages to False and remaining to True.
# Use value "None" for ablation tests.
RUN_STAGES = {"strategies": True,  # True = start run from scratch, gathering strategies and details and candidate code.  False = read from existing candidates*pkl
              "hypertune": True,  # True = hyperparameter tuning active.  False = read from existing hyperresults*pkl.  None = ablated - read preexisting hyperresults*.pkl and update it as if SHORTFLAG had been true.
              "prog_opt_prompt_eval": True,  # True = generate and test optimization candidates.  False = read from existing optresults*.pkl.  None = ablated; skip optimization.
              "full_dev_set": True,  # True = test for FULL_DEV_TIME on NUM_SELECT final candidates.  False = read from existing fullresults*.pkl.  None = ablated; skip and go with existing short timings.
              "open_problems": True} # should always be True
