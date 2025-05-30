"""Covering Array of Strength 3 definitions."""

PROB_NAME = "CA3" # Short abbreviated name for problem
FULL_PROB_NAME = "Covering Array of Strength 3" # Full name for problem in Title Caps
PROB_DEF = "A \"Covering Array of Strength 3\" CA3(N,k,v) is an N x k array (N rows and k columns), with each entry from the v-set of symbols {0,1,...v-1}, so that every N x 3 subarray contains every ordered triple of symbols at least once.  Given (N,k,v), we want to construct CA3(N,k,v).  For our purposes, N<1000, k<1000, and v<6.  Output the CA3(N,k,v) as N lines with one row per line, with each line a space-separated list of k columns where each column is an integer in {0,1,...,v-1}." 

PARAMS = ["N","k","v"] # parameter names
DEV_INSTANCES = [[15,12,2],[16,14,2],[19,21,2],[23,28,2],[24,38,2],[39,7,3],[42,8,3],[45,14,3],[51,16,3],[58,17,3],[59,20,3],[66,22,3],[69,23,3],[69,26,3],[87,39,3],[125,91,3],[64,6,4],[88,8,4],[100,10,4],[110,11,4],[120,12,4],[124,18,4],[127,21,4],[136,22,4],[184,42,4],[232,84,4],[125,6,5],[170,7,5],[185,10,5],[225,12,5],[245,24,5],[385,62,5]]  # PARAMS for each dev instance.  Note this must be <= conf.NUM_PROCESSES
OPEN_INSTANCES = [[183,42,4],[384,62,5],[15,14,2],[18,21,2],[22,28,2],[68,26,3],[86,39,3],[124,91,3],[128,117,3],[231,84,4],[363,273,4],[363,336,4],[484,155,5],[764,651,5],[768,775,5]] # PARAMS for each open-problem instance.  Must be <= conf.NUM_PROCESSES

import itertools
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
            for l in range(j+1,k):
                occur = {}
                for r in range(0,N):
                    triple = (array[r][i],array[r][j],array[r][l])
                    occur[triple] = 1
                for ii in range(0,v):
                    for jj in range(0,v):
                        for ll in range(0,v):
                            triple = (ii,jj,ll)
                            if triple not in occur:
                                return False


    return True                 

