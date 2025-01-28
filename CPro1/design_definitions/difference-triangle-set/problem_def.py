"""Difference Triangle Set definitions."""

PROB_NAME = "DTS" # Short abbreviated name for problem
FULL_PROB_NAME = "Difference Triangle Set" # Full name for problem in Title Caps
PROB_DEF = """A "Difference Triangle Set" (n,k)-DTS is a set X={X_1,...,X_n) where for 1<=i<=n, X_i={a_{i0}, a_{i1},..., a_{ik}} with a_{ij} an integer and with 0 = a_{i0} < a_{i1} <a_{i2} < ... < a_{ik}.  The differences a_{il}-a_{ij} for 1<=i<=n, 0<=j!=l<=k are all distinct and nonzero.  The "scope" of a DTS is the max of all a_{ij} in the DTS.

Given (n,k) and s, we want to construct an (n,k)-DTS with scope s.  The (n,k)-DTS should be represented by an n by k+1 array (n rows and k+1 columns) of elements a_{ij} with 1<=i<=n and 0<=j<=k.
"""

PARAMS = ["n","k","s"] # parameter names
DEV_INSTANCES = [[4,3,24],[3,4,32],[4,4,41],[5,4,51],[6,4,60],[4,5,64],[5,5,79],[4,6,94],[5,6,119],[7,5,113],[4,7,135],[3,8,135],[3,9,184],[6,5,96],[6,6,146],[5,7,171]] # PARAMS for each dev instance.  Note this must be <= conf.NUM_PROCESSES
OPEN_INSTANCES = [[5,7,170],[7,5,112],[6,5,95],[5,6,118],[4,7,134],[3,8,134],[3,9,183],[6,6,145]] # PARAMS for each open-problem instance.  Must be <= conf.NUM_PROCESSES

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
    k = parmlist[1]
    s = parmlist[2]

    array = getsol(soltext,parmlist)
    if len(array)!=n:
        return False
    diffs = {}
    for row in array:
        if len(row)!=k+1:
            return False
        for x in row:
            if x<0 or x>s:
                return False
        if row[0]!=0:
            return False
        for i in range(0,k):
            if row[i]>=row[i+1]:
                return False
            for j in range(i+1,k+1):
                d = row[j]-row[i]
                if d in diffs:
                    return False
                diffs[d] = 1
    return True                 

