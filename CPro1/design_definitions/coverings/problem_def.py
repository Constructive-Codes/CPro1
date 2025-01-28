"""Coverings definitions."""

PROB_NAME = "Cov" # Short abbreviated name for problem
FULL_PROB_NAME = "Covering" # Full name for problem in Title Caps
PROB_DEF = "A \"Covering\" Cov(t,v,k,n) is a pair (X,B), where X is a v-set of elements and B is a collection of k-subsets of X, such that every t-subset of X occurs in at least one block in B.  B has n blocks, and it is required that t<k.  We represent the Covering by an n by v incidence matrix (n rows and v columns) with elements in {0,1}; a 1 in row i column j indicates that block i contains the j'th element.  There are k 1's per row.  Given (t,v,k,n) we want to construct a Cov(t,v,k,n) and provide the incidence matrix."

PARAMS = ["t","v","k","n"] # parameter names
DEV_INSTANCES = [[2,5,3,4],[2,7,4,5],[2,8,4,6],[3,7,4,12],[3,8,4,14],[3,9,4,25],[3,9,5,12],[3,10,5,17],[3,11,5,20],[4,7,5,9],[4,8,6,7],[4,9,6,12],[4,10,6,20],[4,11,7,17],[5,8,6,12],[5,10,7,20],[3,12,5,29],[3,13,5,34],[3,13,6,21],[3,14,6,25],[4,12,6,41],[4,12,7,24],[4,13,7,30],[4,14,8,24],[4,15,9,20],[4,15,10,14],[4,16,10,18],[4,17,11,15],[5,15,10,27],[5,12,7,59],[5,13,8,42],[5,16,11,22]] # PARAMS for each dev instance.  Note this must be <= conf.NUM_PROCESSES
OPEN_INSTANCES = [[3,12,5,28],[3,13,5,33],[3,13,6,20],[3,14,6,24],[4,12,6,40],[4,12,7,23],[4,13,7,29],[4,14,8,23],[4,15,9,19],[4,15,10,13],[4,16,10,17],[4,17,11,14],[5,15,10,26],[5,12,7,58],[5,13,8,41],[5,16,11,21]] # PARAMS for each open-problem instance.  Must be <= conf.NUM_PROCESSES

import utils
import itertools

def getsol(soltext: str,parmlist: list[int]) -> list:
    """Extract solution array from raw program output soltext."""
    # utils.parsesoltext will do the work; just need to specify how many rows (how many lines of text) the solution array will have
    n = parmlist[3]
    array = utils.parsesoltext(soltext,n)
    return array 

def v(soltext: str,parmlist: list[int]) -> bool:
    """Verify that the solution in raw program output soltext is a valid solution with parameters in parmlist."""
    t = parmlist[0]
    v = parmlist[1]
    k = parmlist[2]
    n = parmlist[3]
    array = getsol(soltext,parmlist)
    if len(array)!=n:
        return False
    occur = {}
    for r in array:
        if len(r)!=v:
            return False
        if sum(r)!=k:
            return False
        b = [i for i in range(0,len(r)) if r[i]]
        for x in itertools.combinations(b,t):
            occur[x] = 1
    for x in itertools.combinations(list(range(0,v)),t):
        if not (x in occur):
            return False
    return True                 

