"""Costas Array definitions."""

PROB_NAME = "CA" # Short abbreviated name for problem
FULL_PROB_NAME = "Costas Array" # Full name for problem in Title Caps
PROB_DEF = """A "Costas Array" of order n is an n by n array of dots and blanks that satisfies:
(1) There are n dots and n(n-1) blanks, with exactly one dot in each row and each column.
(2) All the segments between pairs of dots differ in length or slope.
We will represent the Costas Array by a one-dimensional list "CA" of length n, that contains a permutation of {0,1,...,n-1}.  CA[i] identifies the row for the dot that is in column i of the grid.  Condition (1) is automatically satisfied by this representation.  Condition (2) is satisfied if the tuples (j-i,CA[j]-CA[i]) are unique across all j>i with 0<=i,j<n.
Given n, we want to construct the one-dimensional list CA that represents a valid Costas Array of order n."""
PARAMS = ["n"] # parameter names
DEV_INSTANCES = [[3],[4],[5],[6],[7],[8],[9],[10],[11],[12],[13],[14],[15],[16],[17],[18],[19],[20],[21],[22],[23],[24],[25],[26],[27],[28],[29],[30],[31],[34],[35],[36]] # PARAMS for each dev instance.  Note this must be <= conf.NUM_PROCESSES
OPEN_INSTANCES = [[32],[33]] # PARAMS for each open-problem instance.  Must be <= conf.NUM_PROCESSES

import utils

def getsol(soltext: str,parmlist: list[int]) -> list:
    """Extract solution array from raw program output soltext."""
    # utils.parsesoltext will do the work; just need to specify how many rows (how many lines of text) the solution array will have
    array = utils.parsesoltext(soltext,1)
    return array 

def v(soltext: str,parmlist: list[int]) -> bool:
    """Verify that the solution in raw program output soltext is a valid solution with parameters in parmlist."""
    n = parmlist[0]
    arr = getsol(soltext,parmlist)
    CA = arr[0]

    if len(CA)!=n or min(CA)!=0 or max(CA)!=n-1 or len(set(CA))!=n:
        return False

    occur = {}
    for i in range(0,n):
        for j in range(i+1,n):
            t = (j-i,CA[j]-CA[i])
            if t in occur:
                return False
            occur[t] = 1

    return True

