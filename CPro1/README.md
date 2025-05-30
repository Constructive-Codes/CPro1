# CPro1 code

## Requirements for running

- Access to a suitable large language model (LLM).  This can be either:
  - API access, with an API key from one of these providers: OpenAI, Anthropic, Mistral, DeepSeek, DeepInfra.
  - Or a local model, using Ollama.  Note CPro1 makes a lot of LLM calls, so you'll probably want a good GPU that runs your local model quickly.
- If you want to use reasoning models: CPro1 currently works with OpenAI reasoning models (o3-mini, o4-mini, etc.); DeepSeek R1 via DeepSeek's API; and local R1, Qwen3, and QwQ models via Ollama.
- A system running Linux (experiments in the papers used Ubuntu 24.04)
  - At least 16 cores with 32 threads, and 128GB memory, to run the experiments as they were run in the papers.
  - Smaller systems can also work with appropriate configuration (see section "More configuration options" below)
- These tools needs to be installed:
  - Python (experiments in the papers used Python 3.12.3)
  - gcc (experiments in the papers used version 13.3.0)
  - firejail (experiments in the papers used version 0.9.72) - this is for sandboxing the execution of LLM-generated C code.  On Ubuntu you can install with `sudo apt-get install firejail`.
  - Python support for LLM providers.  You can install the required packages with:
```bash
pip install -r requirements.txt
```
If you need to run in a virtual environment (recommended), you can do this with:
```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```
then run as described below.
- You'll need to be patient.  A single run with the default configuration takes about 6-10 days, even with a fast LLM.

## Configure environment variables appropriate to your LLM provider

### API providers:
- Configure your provider: `export PROVIDER="pname"` where pname is one of: OPENAI,MISTRAL,ANTHROPIC,DEEPSEEK,DEEPINFRA
- Set up your API key that you obtained from this provider: `export API_KEY="..."`
- Choose your model, with the name specified by the provider: `export MODEL="...model-name..."` (e.g. MODEL="gpt-4o-2024-05-13")
### Ollama:
```bash
export PROVIDER="OLLAMA"
export API_KEY="ollama"
export MODEL="...ollama-model-name-that-you-have-installed..."
```
- The code assumes that Ollama is running with your model available at http://localhost:11434/v1 - if this URL is not correct for your configuration, you can edit it in utils.py
- Note that Ollama-supplied models can come configured with a context length (e.g. 2048) that is too short for usage with CPro1.  One way to fix this is to create a custom model file.  For example, if you want to use qwen3:30b you can do `ollama show --modelfile qwen3:30b > Modelfile`, then edit Modelfile and after the FROM line add these two lines:
```
PARAMETER num_ctx 40960
PARAMETER num_predict 32768
```
Then save it and do `ollama create qwen3-30b-ctx40k -f Modelfile`.  You can then use your new qwen3-30b-ctx40k as the MODEL.
- If you need your Ollama model to be detected by the code as a reasoning model, the MODEL must contain one of the strings "qwen3", "qwq", or "deepseek-r1".  See "REASONING" below for more information.
- For reasoning models, CPro1 will use LLM parameters (temperature, etc.) as configured in Ollama.

## Running an experiment from the papers

- Create a directory (say it is called "dir") and copy the code from code/*py into it.
- Select from design_definitions the problem_def.py corresponding to the experiment you want to run (e.g. design_definitions/packing-array/problem_def.py for Packing Arrays) and copy it into "dir".
- In "dir", run orch.py - for example:
```bash
python3 orch.py > log 2> errlog &
```
- Note this will run for about 6-10 days (or potentially longer, depending on LLM speed).
- You can monitor the progress in several ways:
  - View the log.  When the run starts, it may take a little while (minutes, or even an hour or more depending entirely on LLM response times) for anything to appear in the log.
  - The errlog will also have information - mostly compiler errors and warnings from gcc for generated code that you can ignore, but if orch.py crashes it may have useful information about the error.
  - Look for *.pkl files.  As orch.py progresses, it will write out several checkpoints:
    - candidates*.pkl with the candidate C code generated by the LLM.
    - hyperresults*.pkl with the results of initial hyperparameter tuning and execution-based scoring for each candidate.
    - optresults*.pkl with the results of optimizing the code for the top (5) candidates.
    - fullresults*.pkl with the results of full (2-hour) testing of each of the top candidates.
  - The C code will also be written to the directory - e.g. if you want to see the code that generated "hyperresult 1006 ..." in the log, look at prog*1006.c
  - The working directory for prog\*X.c is tmpdirprog\*X.  For the full 2-hour testing of the top candidates, and the final 48-hour testing on open instances, this tmpdir will also contain result-*.txt files for any constructed solutions, as soon as they are verified.
  - Note that, when instance-level results are reported on the open instances in the log, the raw output of the code is shown.  This may include invalid solutions - if the solution is valid, the "result for instance" will include a positive rawscore and a positive time.  The result-*.txt files are only written for verified solutions (and those solutions are normalized to a standard format).
- Upon full completion, the results on the open instances will appear in "results1" and "results2" directories for the NUM_FINAL=2 candidates that were run on them.  These directories contain result-*.txt for any instances that were solved, and the C code that generated these results, and the executable compiled from this C code.

## Running your own experiment on your own problem

- Create a directory as above with the code/*py in it, and copy one of the problem_def.py from design_definitions/ as a starting point into your directory as well.
- Edit problem_def.py:
  - Use the examples in design_definitions/ as a guide.
  - Choose a short alphabetic name (e.g. acronym) for PROB_NAME, and put the full name in FULL_PROB_NAME.
  - Your PROB_DEF:
    - Should include the FULL_PROB_NAME and the PROB_NAME and the PARAMS.
    - Should define all details; this definition is the only information the LLM has to work from.
    - Should specify how the solution is to be represented as an array of integers.
  - DEV_INSTANCES: Known solvable instances, for evaluating candidates.  8 or 16 dev instances are good numbers.  Include some small and mid-sized instances, as well as some just below the size of your open instances.
  - OPEN_INSTANCES: Your open instances.  4 or 8 are good numbers.  A small number of OPEN_INSTANCES is good, so that multiple random seeds can be used on each to give more chances of success.
  - Edit **def getsol** so that the last argument it passes to utils.parsesoltext is the number of rows (lines of text) it should extract from candidate program output to represent your solution.  Look at the design_definitions for examples.
  - Edit **def v** to contain your verifier.  The values of your PARAMS will appear in the len(PARAMS) elements of parmlist.  Use **getsol** to obtain the candidate's solution as a 2-d list.  **v** should return True if the solution is valid, False if not.
- Run as described above.  

## Restarting from a checkpoint

- If the process crashes at some point, it can be restarted from the last *.pkl file.
  - Ensure it is really fully crashed though.  There are multiple Python components as well as compiled C code (prog...) that may be left running - all of these should be killed before attempting to restart.
- In conf.py's RUN_STAGES (near the end of the code):
  - If candidates*.pkl is present, set **"strategies": False**
  - If hyperresults*.pkl is present, set **"hypertune": False**
  - If optresults*.pkl is present, set **"prog_opt_prompt_eval": False**
  - If fullresults*.pkl is present, set **"full_dev_set": False**.  At this point, it will only run the NUM_FINAL candidates on the OPEN_INSTANCES.
- It can also be useful to restart from a *.pkl checkpoint after e.g. changing configuration.  For example, to run on different OPEN_INSTANCES, set everything up to and including "full_dev_set" to False, configure your desired OPEN_INSTANCES, and restart.
- When restarting:
  - Use a new log file so you don't lose the old ones - e.g. `python3 orch.py > log2 2> errlog2 &`
  - The prog\*X.c and tmpdirprog\*X from the original run will be overwritten, so make a copy of these somewhere if you want to keep them.

## More configuration options

conf.py has additional configuration options.

The main ones you might want to edit:
- `NUM_PROCESSES = 32`: the number of tests to run in parallel for a candidate.  32 is the right value to rerun experiments from the paper.  But if you have fewer than 32 hardware threads, you may want to try smaller values.  Then you will be restricted in how many DEV_INSTANCES and OPEN_INSTANCES you can run, and you will run fewer seeds per instance since the number of seeds is chosen so that #seeds * #instances <= NUM_PROCESSES.
- `MEM_LIMIT = 3`: This is in GB, per each of NUM_PROCESSES.  So NUM_PROCESSES\*MEM_LIMIT should be under your available memory size - 3\*32=96GB seems to work well for a system with 128GB memory.  Some buggy C code generated by LLMs can rapidly allocate massive amounts of memory, and if MEM_LIMIT isn't low enough then firejail may not kill it in time, leading to the Linux OOM Killer being invoked which can ruin the run.
- `STRATEGY_REPS = 50` : the total number of candidates is NUM_STRATEGES\*STRATEGY_REPS=1000.  You can do smaller or larger runs by changing STRATEGY_REPS.
- `FULL_OPEN_TIME = 172800`: Each of the top NUM_FINAL (=2) candidates is run for this many seconds (48 hours) on the OPEN_INSTANCES - so of the approximately 1-week run, 4 days of it are long runs on the OPEN_INSTANCES.  Shorter times (e.g. 7200; 2 hours) are sometimes enough to get initial results.
- `BASE_SEED = 1000`: If you want to restart from a checkpoint to to try your NUM_FINAL candidates on the OPEN_INSTANCES with more random seeds, you can set this to a different value (e.g. 10000, 20000, ...) to do the additional runs with different seeds.
- `NUM_SELECT = 5` and `NUM_FINAL = 2`: NUM_SELECT candidates proceed to optimization and testing for FULL_DEV_TIME, and then the top NUM_FINAL proceed to testing on OPEN_INSTANCES.

Additional configuration options you could want to edit if you get into the details.

- `FULL_DEV_TIME = 7200`: each of NUM_SELECT candidates is tested on the DEV_INSTANCES for this many seconds.
- `OPT_ROUNDS = 5` and `OPT_TRIES = 50`: optimization generates and tests OPT_TRIES candidates, and then repeats (up to OPT_ROUNDS, though this limit isn't often reached) as long as there is at least MIN_IMPROVE improvement in the score.  Increasing OPT_TRIES could improve optimization results.
- `STRAT_TEMP = 1.0` and `DETAILS_TEMP = 1.0` and PROG_TEMP = 1.0`.  These set the temperature LLM paramater.  Note these are only used for non-reasoning LLMs.  
- `STRATEGY_REPS = 20`: prompt the LLM for lists of this many strategies. 
- `INITHYPERTUNETIME = 0.5`: hyperparameter tuning starts with short 0.5 second runs.  They probably can't really get much shorter than this, since overhead of starting them and stopping them eats up a little time.
- `GRIDSIZE = 1000` and `F = 10`: start  hyperparameter tuning with GRIDSIZE, and each round increase time by a factor of F and decrease grid size by a factor of F.
- `REASONING` is set automatically in conf.py based on model name.  If this doesn't guess correctly for the model you want to use, you may need to set it manually (or add your model name to othe logic in conf.py).

## Ablation

To take a pre-existing completed run with a full set of *.pkl files and run ablation tests as described in the paper:
- Set `"strategies": False` - don't regenerate the candidates.
- For **"Reduce runtime"**, set `FULL_OPEN_TIME = 7200` and
```Python
"hypertune": False
"prog_opt_prompt_eval": False
"full_dev_set": False
```
- For **"No final dev test"**, set `FULL_OPEN_TIME = 7200` and
```Python
"hypertune": False
"prog_opt_prompt_eval": False
"full_dev_set": None
```
- For **"No optimization"**, set `FULL_OPEN_TIME = 7200` and
```Python
"hypertune": False
"prog_opt_prompt_eval": None
"full_dev_set": None
```
- For **"No hyper tuning"**, set `FULL_OPEN_TIME = 7200` and
```Python
"hypertune": None
"prog_opt_prompt_eval": None
"full_dev_set": None
```
