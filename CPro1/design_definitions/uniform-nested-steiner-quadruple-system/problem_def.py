"""Uniform Nested Steiner Quadruple System definitions."""

PROB_NAME = "UNSQS" # Short abbreviated name for problem
FULL_PROB_NAME = "Uniform Nested Steiner Quadruple System" # Full name for problem in Title Caps
PROB_DEF = "A \"Steiner Quadruple System\" (or SQS) of order v consists of a set of blocks, with each block containing 4 elements of the set V={0,1,...,v-1}, such that each subset of 3 elements of V is contained in exactly one block.  There are v*(v-1)*(v-2)/6 subsets of 3 elements of V, and each block covers 4 of these subsets, so the SQS will have v*(v-1)*(v-2)/24 blocks.  A \"Uniform Nested Steiner Quadruple System\" of order v splits each block into two \"ND-pairs\" of two elements each, such that each distinct ND-pair appears the same number of times.  Set p denote the number of distinct ND-pairs that appear among the blocks; it may be that each of the v*(v-1)/2 subsets of 2 elements from V appears as an ND-pair so that p = v*(v-1)/2, or it may be that some subsets of 2 elements from V don't appear as an ND-pair and then we have p < v*(v-1)/2.  There are v*(v-1)*(v-2)/12 ND-pairs, so each of the p distinct ND-pairs appears v*(v-1)*(v-2)/(12*p) times.  We call such a design a UNSQS(v,p).  Given (v,p) we want to construct a UNSQS(v,p).  For our purposes, 8<=v<=58 and 28<=p<=1260.  Output the UNSQS(v,p) as v*(v-1)*(v-2)/24 lines, one block per line, with each line having a space-separated list of 4 numbers identifying the elements of V in the block.  The first ND-pair in a block should be the first two elements listed for the block, and the second ND-pair is the last two elements listed.  Order does not matter within an ND-pair."

PARAMS = ["v","p"] # parameter names
DEV_INSTANCES = [[8,28],[10,30],[16,56],[20,190],[26,325],[28,182],[32,496],[38,703]]  # PARAMS for each dev instance.  Note this must be <= conf.NUM_PROCESSES
OPEN_INSTANCES = [[14,91],[22,154],[26,260],[34,374],[44,946],[46,690],[46,759],[50,1225]] # PARAMS for each open-problem instance.  Must be <= conf.NUM_PROCESSES

import itertools
import utils


def getsol(soltext: str,parmlist: list[int]) -> list:
    """Extract solution array from raw program output soltext."""
    # utils.parsesoltext will do the work; just need to specify how many rows (how many lines of text) the solution array will have
    v = parmlist[0]
    array = utils.parsesoltext(soltext,v*(v-1)*(v-2)//24)
    return array 

def v(soltext: str,parmlist: list[int]) -> bool:
    """Verify that the solution in raw program output soltext is a valid solution with parameters in parmlist."""
    v = parmlist[0]
    p = parmlist[1]
    array = getsol(soltext,parmlist)
    if len(array)!=v*(v-1)*(v-2)//24:
        return False
    pairoccur = {}
    tripleoccur = {}
    for row in array:
        if len(row)!=4 or len(set(row))!=4:
            return False
        for x in row:
            if x<0 or x>=v:
                return False
        pair1 = row[0:2]
        pair2 = row[2:4]
        pair1.sort()
        pair2.sort()
        for pa in [pair1,pair2]:
            if tuple(pa) not in pairoccur:
                pairoccur[tuple(pa)] = 1
            else:
                pairoccur[tuple(pa)] += 1
        triples = list(itertools.combinations(row,3))
        for t in triples:
            st = list(t)
            st.sort()
            if tuple(st) in tripleoccur:
                return False
            else:
                tripleoccur[tuple(st)] = 1

    numndpairs = 0
    for i in range(0,v):
        for j in range(i+1,v):
            pa = [i,j]
            if tuple(pa) in pairoccur:
                numndpairs += 1
                if pairoccur[tuple(pa)]!=v*(v-1)*(v-2)/(12*p):
                    return False

    if numndpairs!=p:
        return False
    
    return True                 


