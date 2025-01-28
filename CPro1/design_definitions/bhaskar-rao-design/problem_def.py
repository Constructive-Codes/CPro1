"""Bhaskar Rao Design definitions."""

PROB_NAME = "BRD" # Short abbreviated name for problem
FULL_PROB_NAME = "Bhaskar Rao Design" # Full name for problem in Title Caps
PROB_DEF = """A "Bhaskar Rao Design" BRD(v,b,r,k,L) is represented by a v by b array (v rows and b columns) with elements a_{ij} in {-1,0,1}.  Each row contains exactly r nonzero elements, and each column contains exactly k nonzero elements.  For any pair of distinct rows, the list of pairwise element products must contain -1 L/2 times, and must contain +1 L/2 times.  That is, for distinct rows f and g, the set of pairwise element products {a_{fj}*a_{gj}} contains -1 L/2 times, +1 L/2 times, and 0 b-L times.

Given (v,b,r,k,L) we want to find a valid BRD(v,b,r,k,L).
"""

PARAMS = ["v","b","r","k","L"] # parameter names
DEV_INSTANCES = [[10,18,9,5,4],[6,6,5,5,4],[11,22,10,5,4],[6,18,15,5,12],[5,16,16,5,16],[6,24,20,5,16],[10,36,18,5,8],[11,44,20,5,8],[10,54,27,5,12],[10,72,36,5,16],[11,66,30,5,12],[15,84,28,5,8],[16,96,30,5,8],[20,76,19,5,4],[21,84,20,5,4],[25,120,24,5,4]] # PARAMS for each dev instance.  Note this must be <= conf.NUM_PROCESSES
OPEN_INSTANCES = [[15,42,14,5,4],[16,48,15,5,4],[26,130,25,5,4],[15,126,42,5,12]] # PARAMS for each open-problem instance.  Must be <= conf.NUM_PROCESSES

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
            if x<-1 or x>1:
                return False

    for i in range(0,v):
        sum = 0
        for j in range(0,b):
            if array[i][j]!=0:
                sum += 1
        if sum!=r:
            return False

    for j in range(0,b):
        sum = 0
        for i in range(0,v):
            if array[i][j]!=0:
                sum += 1
        if sum!=k:
            return False

    for f in range(0,v):
        for g in range(f+1,v):
            numzero = 0
            numpos = 0
            numneg = 0
            for j in range(0,b):
                p = array[f][j]*array[g][j]
                if p==0:
                    numzero += 1
                elif p==-1:
                    numneg += 1
                elif p==1:
                    numpos += 1
                else:
                    return False
            if numneg!=L/2 or numpos!=L/2 or numzero!=b-L:
                return False

    return True                 


