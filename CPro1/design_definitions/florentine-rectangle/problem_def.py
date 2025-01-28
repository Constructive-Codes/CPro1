"""Florentine Rectangle definitions."""

PROB_NAME = "FR" # Short abbreviated name for problem
FULL_PROB_NAME = "Florentine Rectangle" # Full name for problem in Title Caps
PROB_DEF = "A \"Florentine Rectangle\" FR(r,n) is an r x n array (r rows and n columns), with each row having a permutation of the set of symbols S={0,1,2,...,n-1}, such that for any two distinct symbols a and b in S and each m in {1,2,3,...,n-1} there is at most one row in which b appears in the position which is m steps to the right of a.  A single row will have n-m pairs of symbols a,b with b being m steps to the right of a; so n-1 pairs with b directly to the right of a, n-2 with b 2 steps to the right of a, and only 1 pair with b n-1 steps to the right of a.  Given (r,n) we want to construct a FR(r,n)."
PARAMS = ["r","n"] # parameter names
DEV_INSTANCES = [[4,5],[8,9],[10,10],[10,11],[12,12],[12,13],[6,14],[7,14],[6,15],[7,15],[6,20],[7,21],[6,24],[6,25],[6,26],[6,27]] # PARAMS for each dev instance.  Note this must be <= conf.NUM_PROCESSES
OPEN_INSTANCES = [[8,14],[8,15],[7,20],[8,21],[7,24],[7,25],[7,26],[7,27]] # PARAMS for each open-problem instance.  Must be <= conf.NUM_PROCESSES

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
            for i in range(0,n-m):
                if occur[m][r[i]][r[i+m]]>0:
                    return False
                else:
                    occur[m][r[i]][r[i+m]] = 1
    return True                 

