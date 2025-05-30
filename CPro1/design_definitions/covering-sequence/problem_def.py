"""Covering Sequence definitions."""

PROB_NAME = "CS" # Short abbreviated name for problem
FULL_PROB_NAME = "Covering Sequence" # Full name for problem in Title Caps
PROB_DEF = "An \"(n,R)-Covering Sequence\" (abbreviated \"(n,R)-CS\") of length L is a cyclic sequence x_0,x_1,...,x_{L-1} of length L, over the binary alphabet (x_i is in {0,1} for all j) such that for any length-n binary word y_0,y_1,...,y_{n-1} there exists a j such that subsequence x_j,x_{(j+1) mod L},x_{(j+2) mod L}...,x_{(j+n-1) mod L} of length n is Hamming distance at most R away from the word.  That is, y_0,...,y_{n-1} and x_j,...,x_{(j+n-1) mod L} differ in at most R positions.  For our purposes, n <= 16 and R <= 3 and L <= 1200.  Given (n,R,L) we want to construct a (n,R)-CS of length at most L.  Output (n,R)-CS as a list of values in {0,1} separated by spaces, all on one line."

PARAMS = ["n","R","L"] # parameter names
DEV_INSTANCES = [[5,1,8],[6,1,12],[7,1,22],[8,1,32],[8,2,14],[9,2,20],[10,2,38],[10,3,16],[11,3,20],[9,1,93],[10,1,175],[11,1,283],[11,2,111],[12,2,161],[12,3,40],[13,3,93]] # PARAMS for each dev instance.  Note this must be <= conf.NUM_PROCESSES
OPEN_INSTANCES = [[14,2,238],[9,1,92],[10,1,174],[11,1,282],[11,2,110],[12,2,160],[12,3,39],[13,3,92]] # PARAMS for each open-problem instance.  Must be <= conf.NUM_PROCESSES

import itertools
import utils

def getsol(soltext: str,parmlist: list[int]) -> list:
    """Extract solution array from raw program output soltext."""
    # utils.parsesoltext will do the work; just need to specify how many rows (how many lines of text) the solution array will have
    array = utils.parsesoltext(soltext,1)
    return array 

def generate_variants(S, R):
    variants = []
    n = len(S)
    indices = list(range(n))
    # Iterate over all numbers of flips from 0 to R
    for r in range(0, R + 1):
        # Choose r indices at which to flip the bits
        for combo in itertools.combinations(indices, r):
            # Create a mutable list of characters from S
            variant = list(S)
            for i in combo:
                # Flip the bit: '0' -> '1' and '1' -> '0'
                variant[i] = '1' if variant[i] == '0' else '0'
            variants.append(''.join(variant))
    return variants

def v(soltext: str,parmlist: list[int]) -> bool:
    """Verify that the solution in raw program output soltext is a valid solution with parameters in parmlist."""
    n = parmlist[0]
    R = parmlist[1]
    L = parmlist[2]
    array = getsol(soltext,parmlist)
    if len(array)!=1:
        return False
    if len(array[0])>L:
        return False
    covered = {}
    for j in range(0,L):
        s = ""
        for k in range(0,n):
            s = s + str(array[0][(j+k)%L])
        l = generate_variants(s,R)
        for c in l:
            covered[c] = 1
    for x in range(0,(2**n)-1):
        b = bin(x)[2:]
        while len(b)<n:
            b = "0" + b
        if b not in covered:
            return False

    return True                 


