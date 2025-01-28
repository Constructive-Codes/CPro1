"""Balanced Ternary Design definitions."""

PROB_NAME = "BTD" # Short abbreviated name for problem
FULL_PROB_NAME = "Balanced Ternary Design" # Full name for problem in Title Caps
PROB_DEF = """A "Balanced Ternary Design" BTD(V,B;p1,p2,R;K,L) is an arrangement of V elements into B multisets, or blocks, each of cardinality K (K<=V) satisfying:
1. Each element appears R=p1 + 2*p2 times altogether, with multiplicity one in exactly p1 blocks and multiplicity two in exactly p2 blocks.
2. Every pair of distinct elements appears L times; that is, if m_{vb} is the multiplicity of the v'th element in the b'th block, then for every pair of distinct elements v and w, sum_{b=1}^{B} m_{vb} m_{wb} = L.

The BTD is represented by a V by B incidence matrix with elements in {0,1,2}.  The matrix element m_{vb} in the v'th row and b'th column is the multiplicity of the v'th element in the b'th block.  The sum of each row is R, and the sum of each column is K.

Given (V,B,p1,p2,R,K,L) we want to find BTD(V,B;p1,p2,R;K,L).
"""

PARAMS = ["V","B","p1","p2","R","K","L"] # parameter names
DEV_INSTANCES = [[4,8,2,3,8,4,6],[5,10,2,4,10,5,8],[10,10,1,3,7,7,4],[4,12,3,3,9,3,4],[3,3,1,1,3,3,2],[3,4,2,1,4,3,3],[6,6,2,1,4,4,2],[8,20,8,1,10,4,4],[7,14,6,2,10,5,6],[10,22,3,4,11,5,4],[7,21,6,3,12,4,5],[6,18,6,3,12,4,6],[8,24,4,4,12,4,4],[10,25,3,6,15,6,7],[15,15,6,3,12,12,9],[18,18,2,6,14,14,10]] # PARAMS for each dev instance.  Note this must be <= conf.NUM_PROCESSES
OPEN_INSTANCES = [[14,18,7,1,9,7,4],[12,15,6,2,10,8,6],[17,17,8,2,12,12,8],[14,21,6,3,12,8,6],[12,16,4,4,12,9,8],[12,21,4,5,14,8,8],[12,20,4,3,10,6,4],[16,16,7,3,13,13,10]] # PARAMS for each open-problem instance.  Must be <= conf.NUM_PROCESSES

import utils

def getsol(soltext: str,parmlist: list[int]) -> list:
    """Extract solution array from raw program output soltext."""
    # utils.parsesoltext will do the work; just need to specify how many rows (how many lines of text) the solution array will have
    V = parmlist[0]
    array = utils.parsesoltext(soltext,V)
    return array 

def v(soltext: str,parmlist: list[int]) -> bool:
    """Verify that the solution in raw program output soltext is a valid solution with parameters in parmlist."""
    V = parmlist[0]
    B = parmlist[1]
    p1 = parmlist[2]
    p2 = parmlist[3]
    R = parmlist[4]
    K = parmlist[5]
    L = parmlist[6]

    array = getsol(soltext,parmlist)
    if len(array)!=V:
        return False
    for r in array:
        if len(r)!=B:
            return False
        for x in r:
            if x<0 or x>2:
                return False

    for i in range(0,V):
        sum = 0
        for j in range(0,B):
            sum += array[i][j]
        if sum!=R:
            return False

    for j in range(0,B):
        sum = 0
        for i in range(0,V):
            sum += array[i][j]
        if sum!=K:
            return False

    for i in range(0,V):
        mul1 = 0
        mul2 = 0
        for j in range(0,B):
            if array[i][j]==1:
                mul1 += 1
            elif array[i][j]==2:
                mul2 += 1
        if mul1!=p1 or mul2!=p2:
            return False

    for i in range(0,V):
        for k in range(i+1,V):
            sum = 0
            for j in range(0,B):
                sum += array[i][j]*array[k][j]
            if sum!=L:
                return False

    return True                 
