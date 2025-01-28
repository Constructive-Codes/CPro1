"""Circular Florentine Rectangle definitions."""

PROB_NAME = "CFR" # Short abbreviated name for problem
FULL_PROB_NAME = "Circular Florentine Rectangle" # Full name for problem in Title Caps
PROB_DEF = "A \"Circular Florentine Rectangle\" CFR(r,n) is an r x n array (r rows and n columns), with each row having a permutation of the set of symbols S={0,1,2,...,n-1}, such that for any two distinct symbols a and b in S and each m in {1,2,3,...,n-1} there is at most one row in which b appears in the position which is m steps to the right of a.  \"Steps to the right\" is taken circularly - so if a is at position i then b is at position (i+m) mod n.  Given (r,n) we want to construct a CFR(r,n)."

PARAMS = ["r","n"] # parameter names
DEV_INSTANCES = [[2,3],[4,5],[6,7],[2,9],[10,11],[12,13],[4,15],[3,21],[4,21],[5,21],[3,25],[4,25],[3,27],[4,27],[2,33],[3,33]] # PARAMS for each dev instance.  Note this must be <= conf.NUM_PROCESSES
OPEN_INSTANCES = [[6,21],[5,25],[5,27],[4,33]] # PARAMS for each open-problem instance.  Must be <= conf.NUM_PROCESSES

import utils

def getsol(soltext: str,parmlist: list[int]) -> list:
    """Extract solution array from raw program output soltext."""
    # utils.parsesoltext will do the work; just need to specify how many rows (how many lines of text) the solution array will have
    r = parmlist[0]
    array = utils.parsesoltext(soltext,r)
    return array 

def v(soltext: str,parmlist: list[int]) -> bool:
    """Verify that the solution in raw program output soltext is a valid solution with parameters in parmlist."""
    r = parmlist[0]
    n = parmlist[1]
    array = getsol(soltext,parmlist)
    if len(array)!=r:
        return False
    occur = [[[0]*n for i in range(n)] for j in range(n)]
    for r in array:
        if len(r)!=n:
            return False
        for x in r:
            if x<0 or x>=n:
                return False
        if len(r)!=len(set(r)):
            return False
        for m in range(1,n):
            for i in range(0,n):
                if occur[m][r[i]][r[(i+m)%n]]>0:
                    return False
                else:
                    occur[m][r[i]][r[(i+m)%n]] = 1
    return True                 

