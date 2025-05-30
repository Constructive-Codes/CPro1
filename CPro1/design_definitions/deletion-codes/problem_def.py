"""Deletion Code definitions."""

PROB_NAME = "DC" # Short abbreviated name for problem
FULL_PROB_NAME = "Deletion Code" # Full name for problem in Title Caps
PROB_DEF = "A \"Deletion Code\" DC(n,s,m) with parameters (n,s,m) is a set m binary words of length n, such that any two distinct words from the set do not share any any length n-s word obtained by deleting s bits from each of the two words.  Given a length n word x, let D(x) be the set of length n-s words obtained from x by deleting two distinct bits (at positions which need not be adjacent).  Note D(x) has (n choose s) members.  Our requirement is that, for any two distinct words x and y in DC(n,s,m), D(x) and D(y) have the empty intersection.  Given (n,s,m) we want to construct a DC(n,s,m).  For our purposes, 7<=n<=16, s is 2 or 3, and m<250.  Output the DC(n,s,m) as an m by n array, where each row is a space-separated list of n bits in {0,1} representing one word in the Deletion Code."
PARAMS = ["n","s","m"] # parameter names
DEV_INSTANCES = [[8,2,7],[9,2,11],[10,2,16],[11,2,24],[12,2,32],[13,2,49],[14,2,78],[15,2,126],[16,2,201],[10,3,6],[11,3,8],[12,3,12],[13,3,15],[14,3,20],[15,3,28],[16,3,40]] # PARAMS for each dev instance.  Note this must be <= conf.NUM_PROCESSES
OPEN_INSTANCES = [[16,2,204],[13,2,50],[12,2,34]] # PARAMS for each open-problem instance.  Must be <= conf.NUM_PROCESSES

from itertools import combinations
import utils

def getsol(soltext: str,parmlist: list[int]) -> list:
    """Extract solution array from raw program output soltext."""
    # utils.parsesoltext will do the work; just need to specify how many rows (how many lines of text) the solution array will have
    m = parmlist[2]
    array = utils.parsesoltext(soltext,m)
    return array 


def deletions(x: list[int],s: int) -> list[list[int]]:
    n = len(x)
    for keep in combinations(range(n), n - s):
        yield [x[i] for i in keep]

        
def overlap(x: list[int],y: list[int],s: int) -> bool:
    dx = deletions(x,s)
    dy = deletions(y,s)
    dxdict = {}
    for d in dx:
        dxdict[tuple(d)] = 1
    for d in dy:
        if tuple(d) in dxdict:
            return True
    return False


def v(soltext: str,parmlist: list[int]) -> bool:
    """Verify that the solution in raw program output soltext is a valid solution with parameters in parmlist."""
    n = parmlist[0]
    s = parmlist[1]
    m = parmlist[2]
    array = getsol(soltext,parmlist)

    if len(array)!=m:
        return False
    
    for r in array:
        if len(r)!=n:
            return False

    for i in range(0,m):
        for j in range(i+1,m):
            if overlap(array[i],array[j],s):
                print("x: " + str(array[i]) + " / " + str(array[j]))
                return False
            
    return True
    

