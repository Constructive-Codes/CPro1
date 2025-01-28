"""Orthogonal Array definitions."""

PROB_NAME = "OA" # Short abbreviated name for problem
FULL_PROB_NAME = "Orthogonal Array" # Full name for problem in Title Caps
PROB_DEF = "An \"Orthogonal Array\" OA(N,k,s) of size N, degree k, and order s, is a k x N array (k rows and N columns) with entries from the s-set {0,1,...,s-1} having the property that in every 2 x N submatrix, every 2 x 1 column vector appears the same number (lambda) of times.  lambda is called the index of the OA, and lambda = N/s^2.  Given (N,k,s,lambda), we want to construct an OA(N,k,s) with index lambda."

PARAMS = ["N","k","s","lambda"] # parameter names
DEV_INSTANCES = [[4,3,2,1],[8,7,2,2],[9,4,3,1],[18,7,3,2],[27,13,3,3],[16,5,4,1],[32,9,4,2],[45,6,3,5],[45,9,3,5],[45,10,3,5],[36,12,3,4],[36,13,3,4],[48,12,4,3],[48,13,4,3],[100,3,10,1],[100,4,10,1]] # PARAMS for each dev instance.  Note this must be <= conf.NUM_PROCESSES
OPEN_INSTANCES = [[45,11,3,5],[36,14,3,4],[48,14,4,3],[100,5,10,1],[75,9,5,3],[72,8,6,2],[80,11,4,5],[63,14,3,7]] # PARAMS for each open-problem instance.  Must be <= conf.NUM_PROCESSES
                 
import utils

def getsol(soltext: str,parmlist: list[int]) -> list:
    """Extract solution array from raw program output soltext."""
    # utils.parsesoltext will do the work; just need to specify how many rows (how many lines of text) the solution array will have
    k = parmlist[1]
    array = utils.parsesoltext(soltext,k)
    return array 

def v(soltext: str,parmlist: list[int]) -> bool:
    """Verify that the solution in raw program output soltext is a valid solution with parameters in parmlist."""
    N = parmlist[0]
    k = parmlist[1]
    s = parmlist[2]
    l = parmlist[3]
    array = getsol(soltext,parmlist)
    if len(array)!=k:
        return False
    for r in array:
        if len(r)!=N:
            return False
        for x in r:
            if x<0 or x>=s:
                return False

    for i in range(0,k):
        for j in range(i+1,k):
            occur = {}
            for r in range(0,N):
                pair = (array[i][r],array[j][r])
                if not (pair in occur):
                    occur[pair] = 1
                else:
                    occur[pair] += 1
            for p in occur:
                if occur[p]!=l:
                    return False
    return True                 
