"""Tuscan-2 Square definitions."""

PROB_NAME = "T2" # Short abbreviated name for problem
FULL_PROB_NAME = "Tuscan-2 Square" # Full name for problem in Title Caps
PROB_DEF = "A \"Tuscan-2 Square\" T2(n) of size n is an n x n array (n rows and n columns), with each row having a permutation of the set of symbols S={0,1,2,...,n-1}, such that any two distinct symbols a and b in S have exactly one row in which b appears in the position directly to the right of a, and at most one row in which b appears two positions to the right of a (with one symbol between).  Given n, we want to construct a T2(n)."

PARAMS = ["n"] # parameter names
DEV_INSTANCES = [[4],[6],[8],[10],[12]] # PARAMS for each dev instance.  Note this must be <= conf.NUM_PROCESSES
OPEN_INSTANCES = [[11],[13]] # PARAMS for each open-problem instance.  Must be <= conf.NUM_PROCESSES

import utils

def getsol(soltext: str,parmlist: list[int]) -> list:
    """Extract solution array from raw program output soltext."""
    # utils.parsesoltext will do the work; just need to specify how many rows (how many lines of text) the solution array will have
    n = parmlist[0]
    array = utils.parsesoltext(soltext,n)
    return array 

def v(soltext: str,parmlist: list[int]) -> bool:
    """Verify that the solution in raw program output soltext is a valid solution with parameters in parmlist."""
    n = parmlist[0]
    array = getsol(soltext,parmlist)
    if len(array)!=n:
        return False
    occur1 = [[0]*n for i in range(n)]
    occur2 = [[0]*n for i in range(n)]    
    for r in array:
        if len(r)!=n:
            return False
        for x in r:
            if x<0 or x>=n:
                return False
        if len(r)!=len(set(r)):
            return False
        for i in range(0,n-1):
            if occur1[r[i]][r[i+1]]>0:
                return False
            else:
                occur1[r[i]][r[i+1]] = 1
            if i<n-2:
                if occur2[r[i]][r[i+2]]>0:
                    return False
                else:
                    occur2[r[i]][r[i+2]] = 1
    return True                 
