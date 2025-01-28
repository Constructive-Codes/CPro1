"""Runs and scores the program identified in parms.pkl, first doing automated hyperparameter tuning if the program exposes hyperparameters.  Returns results in results.pkl."""
import itertools
import pickle
import math
import sys
import utils
import conf
import multirun

def makegrid(p: list,k: int) -> list[int|float]:
    """Given hyperparameter range in p, return grid of hyperparameter values (linear, then logarithmic at the ends).  k controls the grid size."""
    if k<=1:
        return [p[1],p[2],p[3]]  # min, max, default.  Minimum is 3

    step = (p[2]-p[1])/k
    if isinstance(p[1],int) and isinstance(p[2],int) and isinstance(p[3],int):
        g = [round(p[1] + i*step) for i in range(0,k+1)]
        if p[1]>0:
            m = math.log((p[1]+step)/p[1])/k
            g = [round(p[1]*math.exp(m*i)) for i in range(1,k)] + g
        m = math.log(p[2]/(p[2]-step))/k
        g = g + [round(p[2]+(p[2]-step)*(1-math.exp(m*i))) for i in range(1,k)]
        print(f"starting g {g}")
        g = list(dict.fromkeys(g)) # deduplicate
        print(f"deduplicated g {g}")
    else:
        g = [(p[1] + i*step) for i in range(0,k+1)]
        if p[1]>0:
            m = math.log((p[1]+step)/p[1])/k
            g = [(p[1]*math.exp(m*i)) for i in range(1,k)] + g
        else:
            lower = step/1000.0  # e.g. if first step is 10.0, then log scale goes from 10/1000=0.01 to 10.0
            m = math.log((lower+step)/lower)/k
            g = [(lower*math.exp(m*i)) for i in range(1,k)] + g
        m = math.log(p[2]/(p[2]-step))/k
        g = g + [(p[2]+(p[2]-step)*(1-math.exp(m*i))) for i in range(1,k)]
    return g


if __name__ == "__main__":
    with open("parms.pkl","rb") as f:
        prog,parm_ranges,instances,shortflag = pickle.load(f)

    # generate grid
    if len(parm_ranges)==0:
        grid: list[list[int|float]] = [[]]
    else:
        if shortflag:
            grid = [[r[3] for r in parm_ranges]]
            print(f"ablated grid: {grid}")
        else:
            pergrid = conf.GRIDSIZE
            if len(parm_ranges)>1:
                pergrid = conf.GRIDSIZE//(len(parm_ranges)+1)
            print(f"pergrid {pergrid}")
            # balanced grid
            k = math.floor((pergrid**(1.0/len(parm_ranges)))/3)
            parmgrids = [makegrid(p,k) for p in parm_ranges]
            grid = [list(x) for x in itertools.product(*parmgrids)]
            print(f"after balanced grid with k {k}: {len(grid)}")
            if len(parm_ranges)>1:
                # fine-grained per-parameter grids
                for i in range(0,len(parm_ranges)):
                    parmgrids = []
                    for j,parm in enumerate(parm_ranges):
                        if j!=i:
                            parmgrids = parmgrids + [makegrid(parm,1)]
                        else:
                            parmgrids = parmgrids + [makegrid(parm,math.floor(pergrid/(3**(len(parm_ranges)))))]  # note this is /(3**(len(parm_ranges)-1)) for the number of j!=i parm_ranges, and an additional /3 as usual for makegrid
                    grid = grid + [list(x) for x in itertools.product(*parmgrids)]
                    print(f"after fine-grained for {i} with {math.floor(pergrid/(3**(len(parm_ranges))))}: {len(grid)}")

    # Run prog on the grid hyperparameters.
    results: list = []
    s = 0
    t = conf.INITHYPERTUNETIME
    earlystopflag = False
    while earlystopflag is False and (len(grid)>1 or results==[]):  # do at least one iteration even if len(grid) is 1.
        print(f"starting stage {s} with {len(grid)} candidate hyperparameters")
        sys.stdout.flush()
        results = []
        for p in grid:
            rawresults = multirun.parallelrun(instances,prog,p,conf.SEEDS,t,False) # False = do not write out solutions
            totscore = sum(r[2] for r in rawresults)
            if totscore>0:
                totscore += (sum(r[3]*r[2] for r in rawresults)/sum(r[2] for r in rawresults)) # #solved problems + average time bonus for solved problems
            results += [[[totscore,0,0,utils.instanceresultlist(rawresults)],p]]
        results.sort(reverse=True)
        print(f"stage {s} results:")
        for r in results:
            print(str(r))
        bestscore = results[0][0][0]
        print(f"bestscore {bestscore}")
        if bestscore<1 and len(grid)>1:
            print("stopping early due to score")
            earlystopflag = True
        sys.stdout.flush()
        if shortflag:
            nextnum = 1
        else:
            nextnum = math.ceil(len(results)/conf.F)
        grid = [r[1] for r in results[:nextnum]]
        if len(grid)>1 and earlystopflag is False:
            t = t*conf.F
            s += 1

    finalparms = grid[0]

    finaltime = conf.INITHYPERTUNETIME
    checkgrid = conf.GRIDSIZE
    while checkgrid>1:
        checkgrid //= conf.F
        if checkgrid>1:
            finaltime *= conf.F

    if (finaltime>t) and earlystopflag is False:
        print(f"final run at finaltime {finaltime} for finalparms {finalparms}")
        t = finaltime
        rawresults = multirun.parallelrun(instances,prog,finalparms,conf.SEEDS,t,False) # False = do not write out solutions
        totscore = sum(r[2] for r in rawresults)
        if totscore>0:
            totscore += (sum(r[3]*r[2] for r in rawresults)/sum(r[2] for r in rawresults)) # #solved problems + average time bonus for solved problems
        results[0] = [[totscore,0,0,utils.instanceresultlist(rawresults)],finalparms]

    # Return results for best-scoring hyperparameters.
    with open("./results.pkl","wb") as g:
        pickle.dump([finalparms,t,results[0][0][0],results[0][0][3]],g) # include time limit, score at that time limit, and instances with per-seed timing (-1 for no solution in time limit)
