"""Prompt for optimizing candidate programs, and evaluate the results."""
import sys
import utils
import problem_def
import conf
import prog_multirun


def optcandidates(time_limit: float,hyperparams: list[int|float],timings: list,h: list[list],strategy: str,desc: str,curprogtext: str) -> list[list]:
    """Prompt for optimizations of curprogtext, given timings results obtained within time_limit using hyperparams for hyperparam ranges h.  Return list of updated [prog,messages] candidates."""
    client = utils.connectmodel()
    prompt = f"{problem_def.PROB_DEF}\n\nWe have selected the following approach:\n{strategy}: {desc} Do not terminate until a valid solution is found.\n\nThe {conf.LANGUAGE} code below implements this approach.  It takes command-line parameters: {" ".join(problem_def.PARAMS)} seed (in that order) where seed is the random number generator seed"
    if h != []:
        prompt = prompt + ", followed by additional parameters which represent hyperparameters of the approach"
    prompt = prompt + ".\n\nWe are seeking to make one small change to significantly improve the performance of this code.\n\n\n"
    prompt = prompt + curprogtext + "\n\n\n"

    if h != []:
        prompt += f"With these hyperparameters chosen by hyperparameter tuning: {utils.hyperparamtext(h,hyperparams)}.  "
    prompt += f"This code {utils.timingtext(timings,time_limit,problem_def.PARAMS)}"

    prompt = prompt + ".  We want to make one small change to the code to significantly improve performance, without parallelizing or changing the algorithm"
    if h != []:
        prompt = prompt + " or the hyperparameter handling"
    prompt = prompt + ".  Describe your plan for making one small change to significantly improve performance.  Then, implement your plan and provide the complete updated code."

    m = utils.addmessage([],"user",prompt)
    print(f"{prompt}\n")

    newcandidates: list[list] = []

    for tries in range(0,conf.OPT_TRIES):
        print(f"try number {tries}:\n")
        result = utils.runprompt(m,conf.PROG_TEMP,conf.TOP_P,client)
        print(f"{result}\n")

        newm = utils.addmessage(m,"assistant",result)

        program = utils.extractprogram(result,conf.LANGUAGE)
        if program=="":
            print("NOTICE: no program found")
        else:
            print(f"{program}\n")
            newcandidates = newcandidates + [[program,newm]]

    return newcandidates


def optrun(r: int,runlabel: str,time_limit: float,hyperparams: list[int|float],instance_timings: list,parm_ranges: list[list],strat: str,strat_summary: str,progtext: str,score: float) -> list:
    """Prompt for optimizations of progtext, given chosen hyperparams & prior score/instance_timings.  r is initial run num.  Eval opt candidates.  Return best updated [program,score,timing,r]; r is final run num."""
    safetylimit = time_limit*3  # Time limit to stop unusual runaway jobs.  Includes substantial overhead.
    continueflag = True
    optround = 0
    while continueflag and optround<conf.OPT_ROUNDS:
        # create candidates
        print(f"optimization round {optround}")
        optcand = optcandidates(time_limit,hyperparams,instance_timings,parm_ranges,strat,strat_summary,progtext)

        # eval candidates
        bestscore = -1.0
        bestcandidate = None
        for d in optcand:
            newprogtext = d[0]
            print(f"starting new {r} ({strat})")
            sys.stdout.flush()
            result = prog_multirun.multirun(runlabel,r,newprogtext,'multirun.py',safetylimit,[hyperparams,conf.OPT_SEEDS,time_limit,problem_def.DEV_INSTANCES]) # final list is passed through to multirun.py
            if result is not None:
                optresults = result[0]
                optscore = utils.computescore(optresults)
                print(f"finished {r} ({strat}) - score {optscore} instance_timings {[[f[0],f[4]] for f in optresults]}")
                if optscore>bestscore:
                    bestscore = optscore
                    bestresults = optresults
                    bestcandidate = d
            else:
                print(f"finished {r} ({strat}) - failure")
            sys.stdout.flush()
            r += 1

        print(f"starting orig {r} ({strat})")
        sys.stdout.flush()
        result = prog_multirun.multirun(runlabel,r,progtext,'multirun.py',safetylimit,[hyperparams,conf.OPT_SEEDS,time_limit,problem_def.DEV_INSTANCES]) # final list is passed through to multirun.py
        if result is not None:
            origresults = result[0]
            origscore = utils.computescore(origresults)
            print(f"finished orig {r} ({strat}) - score {origscore} instance_timings {[[f[0],f[4]] for f in origresults]}")
        else:
            print(f"finished orig {r} ({strat}) - failure")
        sys.stdout.flush()
        r += 1

        if bestscore>origscore+conf.MIN_IMPROVE and bestcandidate is not None:
            print(f"improved from {origscore} to {bestscore} timings {[[f[0],f[4]] for f in bestresults]}")
            progtext = bestcandidate[0]   # update program
            score = bestscore
            instanceresultlist = utils.instanceresultlist(bestresults)
            print(f"constructed instanceresultlist {instanceresultlist}")
            instance_timings = instanceresultlist
            optround += 1
        else:
            print(f"failed to improve adequately: best was from {origscore} to {bestscore} timings {[[f[0],f[4]] for f in bestresults]}")
            continueflag = False
    return [progtext,score,instance_timings,r]
