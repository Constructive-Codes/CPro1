"""Supersimple Balanced Incomplete Block Design definitions.""" 

PROB_NAME = "SBIBD" # Short abbreviated name for problem
FULL_PROB_NAME = "Supersimple Balanced Incomplete Block Design" # Full name for problem in Title Caps
PROB_DEF = """A "Supersimple Balanced Incomplete Block Design" SBIBD(v,b,r,k,L) is a pair (V,B) where V is a v-set and B is a collection of b k-subsets of V (blocks) such that each element of V is contained in exactly r blocks, any 2-subset of V is contained in exactly L blocks, and any two distinct blocks have at most two elements in common.
The SBIBD is represented by a v by b incidence matrix (v rows and b columns) with elements in {0,1}.  The matrix element m_{ij} in the i'th row and j'th column is 1 iff element i is contained in block j.  The sum of each row is r, and the sum of each column is k.  For each pair of distinct elements y and z, sum_{j=1}^{b} m_{yj} m_{zj} = L.  For each pair of distinct blocks g and h, sum_{i=1}^{v} m_{ig} m_{ih} <= 2.

Given (v,b,r,k,L) we want to find the incidence matrix for a valid SBIBD(v,b,r,k,L).
"""

PARAMS = ["v","b","r","k","L"] # parameter names
DEV_INSTANCES = [[9,18,8,4,3],[7,7,4,4,2],[11,11,5,5,2],[12,33,11,4,3],[16,16,6,6,2],[15,42,14,5,4],[16,40,10,4,2],[16,48,15,5,4],[14,91,26,4,6],[21,63,15,5,3],[21,84,20,5,4],[25,60,12,5,2],[17,68,20,5,5],[20,76,19,5,4],[15,105,28,4,6],[22,77,21,6,5]] # PARAMS for each dev instance.  Note this must be <= conf.NUM_PROCESSES
OPEN_INSTANCES = [[21,105,25,5,5],[21,126,30,5,6],[25,90,18,5,3],[21,42,12,6,3],[24,92,23,6,5],[26,65,15,6,3],[31,93,18,6,3],[29,58,14,7,3]] # PARAMS for each open-problem instance.  Must be <= conf.NUM_PROCESSES

import utils

def getsol(soltext: str,parmlist: list[int]) -> list:
    """Extract solution array from raw program output soltext."""
    # utils.parsesoltext will do the work; just need to specify how many rows (how many lines of text) the solution array will have
    v = parmlist[0]
    array = utils.parsesoltext(soltext,v)
    return array 

def v(soltext: str,parmlist: list[int]) -> bool:
    """Verify that the solution in raw program output soltext is a valid solution with parameters in parmlist."""
    v = parmlist[0]
    b = parmlist[1]
    r = parmlist[2]
    k = parmlist[3]
    L = parmlist[4]

    array = getsol(soltext,parmlist)
    if len(array)!=v:
        return False
    for row in array:
        if len(row)!=b:
            return False
        for x in row:
            if x<0 or x>1:
                return False

    for i in range(0,v):
        sum = 0
        for j in range(0,b):
            sum += array[i][j]
        if sum!=r:
            return False

    for j in range(0,b):
        sum = 0
        for i in range(0,v):
            sum += array[i][j]
        if sum!=k:
            return False

    for i in range(0,v):
        for k in range(i+1,v):
            sum = 0
            for j in range(0,b):
                sum += array[i][j]*array[k][j]
            if sum!=L:
                return False

    for g in range(0,b):
        for h in range(g+1,b):
            sum = 0
            for i in range(0,v):
                sum += array[i][g]*array[i][h]
            if sum>2:
                return False

    return True                 


