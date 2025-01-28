"""Toplevel orchestrator.  Run this after setting up conf.py and problem_def.py."""
import pickle
import sys
import math
import subprocess
import strats
import opt_prompt_eval
import prog_multirun
import utils
import problem_def
import conf
import candidate

RUNLABEL = problem_def.PROB_NAME + str(conf.RUN_LABEL_NUM)

# Generate strategies, and then details and a candidate program for each strategy.
if conf.RUN_STAGES["strategies"] is True:
    candidates: list[candidate.Candidate] = []
    for rep in range(0,conf.STRATEGY_REPS):
        print(f"starting strategy rep {rep}\n")
        strategies = strats.get_strategies(problem_def.DEV_INSTANCES[0],problem_def.OPEN_INSTANCES[0])
        candidates = candidates + strats.get_detailsprograms(problem_def.DEV_INSTANCES[0],problem_def.OPEN_INSTANCES[0],strategies)
    with open(f"candidates_{RUNLABEL}.pkl","wb") as g:
        pickle.dump(candidates,g)
else:
    with open(f"candidates_{RUNLABEL}.pkl","rb") as f:
        candidates = pickle.load(f)


# Score each candidate, after doing automated hyperparameter tuning for candidates that expose hyperparameters.
r = conf.INIT_RUN_NUM
if conf.RUN_STAGES["hypertune"] is False:
    with open(f"hyperresults_{RUNLABEL}.pkl","rb") as f:
        hyperresults = pickle.load(f)
else:
    if conf.RUN_STAGES["hypertune"] is None: # ablate a prior run
        with open(f"hyperresults_{RUNLABEL}.pkl","rb") as f:
            candidates = pickle.load(f)
    hyperresults = []
    for c in candidates:
        if conf.RUN_STAGES["hypertune"] is None and c.parm_ranges==[]:
            print(f"keeping {r} ({c.strat}): parm_ranges {c.parm_ranges} score {c.score} at time_limit {c.time_limit} instance_timings {c.instance_timings}")
            hyperresults += [c]
            sys.stdout.flush()
        else:
            if conf.RUN_STAGES["hypertune"] is None:
                print(f"starting ablated {r} ({c.strat}): parm_ranges {c.parm_ranges}")
            else:
                print(f"starting {r} ({c.strat}): parm_ranges {c.parm_ranges}")
            sys.stdout.flush()
            safetylimit = 3*conf.INITHYPERTUNETIME*conf.GRIDSIZE*int((math.log(conf.GRIDSIZE)/math.log(conf.F))+1)  # Safety time limit to stop unusual runaway jobs.  Allows for substantial overhead time.
            result = prog_multirun.multirun(RUNLABEL,r,c.progtext,'hyper_sub_multi.py',safetylimit,[c.parm_ranges,problem_def.DEV_INSTANCES,conf.SHORTFLAG or conf.RUN_STAGES["hypertune"] is None]) # final list is passed through to hyper_sub_multi.py
            if result is not None:
                [c.hyperparams,c.time_limit,c.score,c.instance_timings],logtext = result
                hyperresults += [c]
                print(f"hyperresult {r} ({c.strat}): score {c.score} at time_limit {c.time_limit} hyperparams {c.hyperparams} instance_timings {c.instance_timings}")
            else:
                print(f"hyperresult {r} ({c.strat}): failure")
            sys.stdout.flush()
        r += 1
    with open(f"hyperresults_{RUNLABEL}.pkl","wb") as g:
        pickle.dump(hyperresults,g)


# For the best conf.NUM_SELECT scores, prompt for optimization candidates and test them pick the best.  Repeat if optimization is successful.
if conf.RUN_STAGES["prog_opt_prompt_eval"] is True:
    # choose candidates
    hyperresults.sort(key=lambda c: c.score, reverse=True)
    selected = hyperresults[:conf.NUM_SELECT]
    selectednum = 0
    for c in selected:
        print(f"optimization for {selectednum} initial score {c.score} strategy ({c.strat}) hyperparams {c.hyperparams}\n")
        c.progtext,c.score,c.instance_timings,r = opt_prompt_eval.optrun(r,RUNLABEL,c.time_limit,c.hyperparams,c.instance_timings,c.parm_ranges,c.strat,c.strat_summary,c.progtext,c.score)
        # note that selected[selectednum] now already has the updated c
        selectednum += 1
    with open(f"optresults_{RUNLABEL}.pkl","wb") as g:
        pickle.dump(selected,g)
elif conf.RUN_STAGES["prog_opt_prompt_eval"] is False:  # not running - read in previously saved results
    with open(f"optresults_{RUNLABEL}.pkl","rb") as f:
        selected = pickle.load(f)
else: # Skip over this stage completely and go straight to next stage.
    hyperresults.sort(key=lambda c: c.score, reverse=True)
    selected = hyperresults[:conf.NUM_SELECT]  # still need to select results for next stage.


# Run the best conf.NUM_SELECT candidates on the problem_def.DEV_INSTANCES for a longer conf.FULL_DEV_TIME.
if conf.RUN_STAGES["full_dev_set"] is True:
    fullresults = []
    for c in selected:
        print(f"starting {r} ({c.strat}) hyperparams {c.hyperparams}")
        sys.stdout.flush()
        safetylimit = conf.FULL_DEV_TIME*3 # safety limit to stop unusual runaway jobs
        result = prog_multirun.multirun(RUNLABEL,r,c.progtext,'multirun.py',safetylimit,[c.hyperparams,conf.FULL_DEV_SEEDS,conf.FULL_DEV_TIME,problem_def.DEV_INSTANCES]) # final list is passed through to multirun.py
        if result is not None:
            instanceresults = result[0]
            c.full_dev_score = utils.computescore(instanceresults)
            print(f"finished {r} ({c.strat}) hyperparams {c.hyperparams} - score {c.full_dev_score} instance_timings {[[f[0],f[4]] for f in instanceresults]}")
            c.full_dev_instance_timings = utils.instanceresultlist(instanceresults)
            fullresults += [c]
        else:
            print(f"finished {r} ({c.strat}) hyperparams {c.hyperparams} - failure")
        r += 1
        sys.stdout.flush()
    fullresults.sort(key=lambda c: c.full_dev_score, reverse=True)
    fullselected = fullresults[:conf.NUM_FINAL]
    with open(f"fullresults_{RUNLABEL}.pkl","wb") as g:
        pickle.dump(fullselected,g)
elif conf.RUN_STAGES["full_dev_set"] is False: # not running - read in previously saved results
    with open(f"fullresults_{RUNLABEL}.pkl","rb") as f:
        fullselected = pickle.load(f)
else: # Skip over this stage completely and go straight to next stage.
    selected.sort(key=lambda c: c.score, reverse=True)  # still need to select results for next stage.
    fullselected = selected[:conf.NUM_FINAL]


# Run the best conf.NUM_FINAL candidates on the problem_def.OPEN_INSTANCES for conf.FULL_OPEN_TIME.
if conf.RUN_STAGES["open_problems"]:
    resultdirnum = 1
    for c in fullselected:
        print(f"starting {r} ({c.strat}) hyperparams {c.hyperparams}")
        sys.stdout.flush()
        safetylimit = conf.FULL_OPEN_TIME*3 # safety limit to stop unusual runaway jobs
        result = prog_multirun.multirun(RUNLABEL,r,c.progtext,'multirun.py',safetylimit,[c.hyperparams,conf.FULL_OPEN_SEEDS,conf.FULL_OPEN_TIME,problem_def.OPEN_INSTANCES]) # final list is passed through to multirun.py
        if result is not None:
            c.open_problem_results = result[0]
            c.open_problem_score = utils.computescore(c.open_problem_results)
            print(f"finished {r} ({c.strat}) hyperparams {c.hyperparams} totscore {c.open_problem_score}")
            for openresult in c.open_problem_results:
                print(f"result for instance {openresult[0]} seed {openresult[1]} rawscore {openresult[2]} bonus {openresult[3]} time {openresult[4]}\n")
                print(f"{openresult[5]}\n")
        else:
            print(f"finished {r} ({c.strat}) hyperparams {c.hyperparams} - failure")
        sys.stdout.flush()
        # copy code and results to a final results# dir
        subprocess.run(f"mkdir results{resultdirnum}",shell=True)
        progname = f"prog{RUNLABEL}r{r}"
        subprocess.run(f"cp ./tmpdir{progname}/{progname}.c ./results{resultdirnum}",shell=True) # source code
        subprocess.run(f"cp ./tmpdir{progname}/{progname} ./results{resultdirnum}",shell=True)   # compiled code
        subprocess.run(f"cp ./tmpdir{progname}/result-*.txt ./results{resultdirnum}",shell=True) # any positive results
        r += 1
        resultdirnum += 1
