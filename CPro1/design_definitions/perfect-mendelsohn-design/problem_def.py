"""Perfect Mendelsohn Design definitions."""

PROB_NAME = "PMD" # Short abbreviated name for problem
FULL_PROB_NAME = "Perfect Mendelsohn Design" # Full name for problem in Title Caps
PROB_DEF = """Given a k-tuple (x_0, x_1, x_2, ..., x_{k-1}), elements x_i, x_{i+t} are t-apart in the k-tuple, where i+t is taken modulo k.

A "Perfect Mendelsohn Design" with parameters v and k is denoted as a (v,k)-PMD.  It is a set V={0,1,...,v-1} of size v together with a collection B of blocks of ordered k-tuples of distinct elements from V, such that for every i=1,2,3,...,k-1 each ordered pair (x,y) of distinct elements from V is i-apart in exactly one block.  Note since there are v*(v-1) pairs of distinct elements that must be 1-apart in exactly one block, and each block has k pairs that are 1-apart, the design will contain b=v*(v-1)/k blocks.  We will use b to denote the number of blocks.  

Given (v,k,b), we want to construct a (v,k)-PMD with b blocks.  We will represent the PMD with a b by k array (b rows and k columns}, with each element of the array chosen from {0,1,...,v-1}."""

PARAMS = ["v","k","b"] # parameter names
DEV_INSTANCES = [[7,3,14],[5,4,5],[7,6,7],[8,7,8],[9,4,18],[9,3,24],[10,3,30],[11,5,22],[12,3,44],[13,3,52],[12,4,33],[13,4,39],[13,6,26],[15,5,42],[16,4,60],[16,5,48]] # PARAMS for each dev instance.  Note this must be <= conf.NUM_PROCESSES
OPEN_INSTANCES = [[9,6,12],[10,6,15],[12,6,22],[15,6,35],[16,6,40],[18,6,51],[14,7,26],[15,7,30]] # PARAMS for each open-problem instance.  Must be <= conf.NUM_PROCESSES

import utils


def getsol(soltext: str,parmlist: list[int]) -> list:
    """Extract solution array from raw program output soltext."""
    # utils.parsesoltext will do the work; just need to specify how many rows (how many lines of text) the solution array will have
    b = parmlist[2]
    array = utils.parsesoltext(soltext,b)
    return array 

def v(soltext: str,parmlist: list[int]) -> bool:
    """Verify that the solution in raw program output soltext is a valid solution with parameters in parmlist."""
    v = parmlist[0]
    k = parmlist[1]
    b = parmlist[2]
    array = getsol(soltext,parmlist)
    if len(array)!=b:
        return False
    for r in array:
        if len(r)!=k:
            return False
        check = {}
        for x in r:
            if x<0 or x>=v:
                return False
            if x in check:
                return False
            check[x] = 1

    occur = {}
    for r in array:
        for t in range(1,k):
            for i in range(0,k):
                pair = (t,r[i],r[(i+t)%k])
                if pair in occur: # repeat occurrence
                    return False
                occur[pair] = 1

    return True                 

