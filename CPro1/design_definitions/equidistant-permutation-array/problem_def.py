"""Equidistant Permutation Array definitions."""

PROB_NAME = "EPA" # Short abbreviated name for problem
FULL_PROB_NAME = "Equidistant Permutation Array" # Full name for problem in Title Caps
PROB_DEF = "An \"equidistant permutation array\" (EPA) with parameters (n,d,m) can be represented as an m by n matrix (m rows and n columns), where each row is the permutation of the numbers 0 to n-1.  Each pair of distinct rows must differ in exactly d positions.  Given (n,d,m), we want to construct an equidistant permutation array (EPA) with these parameters."
PARAMS = ["n","d","m"] # parameter names
DEV_INSTANCES = [[6, 4, 5], [6, 5, 10], [8, 5, 10], [12, 4, 6], [7, 6, 13], [8, 6, 13], [8, 7, 16], [9, 7, 16]] # PARAMS for each dev instance.  Note this must be <= conf.NUM_PROCESSES
OPEN_INSTANCES = [[10, 7, 18], [11, 7, 18], [9, 8, 21], [10, 8, 21], [11, 8, 21], [12, 8, 21]] # PARAMS for each open-problem instance.  Must be <= conf.NUM_PROCESSES

import utils

def getsol(soltext: str,parmlist: list[int]) -> list:
    """Extract solution array from raw program output soltext."""
    # utils.parsesoltext will do the work; just need to specify how many rows (how many lines of text) the solution array will have
    m = parmlist[2]
    array = utils.parsesoltext(soltext,m)
    return array 

def dist(l1,l2):
    distance = 0
    for i in range(0,len(l1)):
        if l1[i]!=l2[i]:
            distance += 1
    return distance

def v(soltext: str,parmlist: list[int]) -> bool:
    """Verify that the solution in raw program output soltext is a valid solution with parameters in parmlist."""
    n = parmlist[0]
    d = parmlist[1]
    m = parmlist[2]
    array = getsol(soltext,parmlist)

    if len(array)!=m:
        return False
    
    for r in array:
        if len(r)!=n:
            return False
        if set(r)!=set(range(n)):
            return False
        
    for i in range(0,m):
        for j in range(i+1,m):
            if dist(array[i],array[j])!=d:
                return False
            
    return True
    




