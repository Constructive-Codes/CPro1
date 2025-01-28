"""Balanced Incomplete Block Design definitions."""

PROB_NAME = "BIBD" # Short abbreviated name for problem
FULL_PROB_NAME = "Balanced Incomplete Binary Design" # Full name for problem in Title Caps
PROB_DEF = """A "Balanced Incomplete Block Design" BIBD(v,b,r,k,L) is a pair (V,B) where V is a v-set and B is a collection of b k-subsets of V (blocks) such that each element of V is contained in exactly r blocks and any 2-subset of V is contained in exactly L blocks.
The BIBD is represented by a v by b incidence matrix (v rows and b columns) with elements in {0,1}.  The matrix element m_{ij} in the i'th row and j'th column is 1 iff element i is contained in block j.  The sum of each row is r, and the sum of each column is k.  For each pair of distinct elements y and z, sum_{j=1}^{b} m_{yj} m_{zj} = L.

Given (v,b,r,k,L) we want to find the incidence matrix for a valid BIBD(v,b,r,k,L).
"""

PARAMS = ["v","b","r","k","L"] # parameter names
DEV_INSTANCES = [[9,12,4,3,1],[10,15,6,4,2],[7,14,6,3,2],[16,16,6,6,2],[8,14,7,4,3],[9,24,8,3,2],[25,50,8,4,1],[9,18,8,4,3],[10,30,9,3,2],[7,21,9,3,3],[28,63,9,4,1],[10,18,9,5,4],[6,20,10,3,4],[12,22,11,6,5],[19,57,12,4,2],[41,82,10,5,1]] # PARAMS for each dev instance.  Note this must be <= conf.NUM_PROCESSES
OPEN_INSTANCES = [[40,52,13,10,3],[51,85,10,6,1],[39,57,19,13,6],[40,60,21,14,7]] # PARAMS for each open-problem instance.  Must be <= conf.NUM_PROCESSES

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

    return True                 

