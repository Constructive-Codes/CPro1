"""Import to use parallelrun(), or run directly to do the parallel run specified in parms.pkl."""
import time
import subprocess
import re
import pickle
from multiprocessing import Pool
import utils
import problem_def
import conf


def objective(instance: str,seed: int,prog: str,parms: list,time_limit: float,writeflag: bool) -> list:
    """Run prog on parms with time_limit and score the result.  Write results to disk if writeflag."""
    print(f"running {instance} {seed}")
    parmstring = " ".join(str(p) for p in parms)
    rawscore = 0
    bonus = 0.0
    command = f"timeout {time_limit} ./{prog} {instance} {seed} {parmstring}"
    start_time = time.time()
    result = subprocess.run(command,shell=True,capture_output=True,text=True)
    end_time = time.time()
    diff = end_time-start_time
    if re.search(r'[0-9]+.*[^0-9].*[0-9]+.*[^0-9].*[0-9]+',result.stdout):
        if problem_def.v(result.stdout,[int(p) for p in instance.split()]):
            bonus = max(0.0,(time_limit-diff)/time_limit)
            rawscore += 1
            if writeflag:
                fname = "-".join(str(p) for p in instance.split())
                fname = f"result-{fname}-seed{seed}.txt"
                with open(fname,"w",encoding="utf-8") as g:
                    g.write(utils.prettystring(problem_def.getsol(result.stdout,[int(p) for p in instance.split()])))
        else:
            diff = -1  # failure
    else:
        diff = -1
    return [instance,seed,rawscore,bonus,diff,result.stdout]


def parallelrun(instances: list[list[int]],prog: str,hyperparams: list,numseeds: int,time_limit: float,writeflag: bool) -> list:
    """Run prog numseeds times on each of instances, in parallel."""
    instancestrings = [" ".join([str(x) for x in l]) for l in instances]
    jobs = []
    for i in instancestrings:
        for seed in range(conf.BASE_SEED,conf.BASE_SEED+numseeds):
            jobs.append((i,seed,prog,hyperparams,time_limit,writeflag))
    with Pool(processes=conf.NUM_PROCESSES) as pool:
        results = pool.starmap(objective,jobs)
    return results


# This module can be run directly, or can be imported to use parallelrun()
if __name__ == "__main__":
    with open("parms.pkl","rb") as f:
        prog,hyperparams,numseeds,time_limit,instances = pickle.load(f)
    results = parallelrun(instances,prog,hyperparams,numseeds,time_limit,True) # True = write solutions
    with open("./results.pkl","wb") as g:
        pickle.dump(results,g)
