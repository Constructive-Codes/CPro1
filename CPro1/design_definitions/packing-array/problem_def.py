"""Packing Array definitions."""

PROB_NAME = "PA" # Short abbreviated name for problem
FULL_PROB_NAME = "Packing Array" # Full name for problem in Title Caps
PROB_DEF = "A \"Packing Array\" PA(N,k,v) is an N x k array (N rows and k columns), with each entry from the v-set {0,1,...v-1}, so that every N x 2 subarray contains every ordered pair of symbols at most once.  Given (N,k,v), we want to construct PA(N,k,v)." 
PARAMS = ["N","k","v"] # parameter names
DEV_INSTANCES = [[4,6,3], [6,5,3], [9,6,4], [20,6,5], [25,6,5], [25,5,6], [30,5,6], [31,5,6], [16,7,6], [23,7,6], [17,10,8], [22,10,8], [25,10,8], [14,11,8], [19,11,8], [22,11,8]] # PARAMS for each dev instance.  Note this must be <= conf.NUM_PROCESSES
OPEN_INSTANCES = [[32,5,6], [24,7,6], [26,10,8], [23,11,8]] # PARAMS for each open-problem instance.  Must be <= conf.NUM_PROCESSES

import utils

def getsol(soltext: str,parmlist: list[int]) -> list:
    """Extract solution array from raw program output soltext."""
    # utils.parsesoltext will do the work; just need to specify how many rows (how many lines of text) the solution array will have
    N = parmlist[0]
    array = utils.parsesoltext(soltext,N)
    return array

def v(soltext: str,parmlist: list[int]) -> bool:
    """Verify that the solution in raw program output soltext is a valid solution with parameters in parmlist."""
    N = parmlist[0]
    k = parmlist[1]
    v = parmlist[2]
    array = getsol(soltext,parmlist)
    if len(array)!=N:
        return False
    for r in array:
        if len(r)!=k:
            return False
        for x in r:
            if x<0 or x>=v:
                return False

    for i in range(0,k):
        for j in range(i+1,k):
            occur = {}
            for r in range(0,N):
                pair = (array[r][i],array[r][j])
                if pair in occur: # repeat occurrence
                    return False
                occur[pair] = 1

    return True                 

