"""Johnson Clique Cover definitions."""

PROB_NAME = "JCC" # Short abbreviated name for problem
FULL_PROB_NAME = "Johnson Clique Cover" # Full name for problem in Title Caps
PROB_DEF = "The Johnson Graph J(N,k) with k<=N/2 is the graph whose vertices are k-element subsets of [N]={1,2,...,N}, with two subsets connected by an edge if their intersection has size exactly k-1.  A \"Johnson Clique Cover\" JCC(N,k,C) of size C is a set of C cliques in J(N,k), such that the union of these cliques includes all vertices in J(N,k).  Note the cliques in the clique cover do not need to be disjoint; they may share vertices.  For our purposes, N<=15 and C<800.  Given (N,k,C) we want to construct a JCC(N,k,C).  It is a theorem that it suffices to consider clique covers that consist only of maximal cliques, and that all maximal cliques of J(N,k) are either type 0 or type 1, defined as follows.  A type 0 clique specifies a k-1 element subset S of [N], and consists of all vertices corresponding to the subset S plus x, for each x that is an element of [N] that is not in S.  A type 1 clique specifies k+1 elements in [N], and consists of all vertices corresponding to the subset S excluding x, for each x that is an element of S.  Specify a clique as a space-separated list of integers where the first integer is the type (0 or 1) and the remaining integers specify the elements of S.  For example in J(5,2) the type 0 clique \"0 1\" consists of vertices for the subsets {1,2}, {1,3}, {1,4}, and {1,5}; and the type 1 clique \"1 3 4 5\" consists of vertices for the subsets {4,5}, {3,4}, and {3,5}.  Output the clique cover as C lines, with one clique per line, where each line is a space-separated list of integers starting with the type (0 or 1)."

PARAMS = ["N","k","C"] # parameter names
DEV_INSTANCES = [[5,3,3],[8,4,14],[9,4,25],[10,4,40],[10,5,46],[11,4,56],[11,5,77],[12,4,76],[12,5,115],[13,4,110],[13,5,185],[13,6,280],[13,6,264],[14,4,141],[14,5,259],[14,5,229]]  # PARAMS for each dev instance.  Note this must be <= conf.NUM_PROCESSES
OPEN_INSTANCES = [[11,5,76],[12,4,75],[12,5,114],[13,4,109],[13,5,184],[13,6,263],[14,5,228],[15,4,179]] # PARAMS for each open-problem instance.  Must be <= conf.NUM_PROCESSES

import itertools
import utils

def getsol(soltext: str,parmlist: list[int]) -> list:
    """Extract solution array from raw program output soltext."""
    # utils.parsesoltext will do the work; just need to specify how many rows (how many lines of text) the solution array will have
    C = parmlist[2]
    array = utils.parsesoltext(soltext,C)
    return array 

def v(soltext: str,parmlist: list[int]) -> bool:
    """Verify that the solution in raw program output soltext is a valid solution with parameters in parmlist."""
    N = parmlist[0]
    k = parmlist[1]
    C = parmlist[2]
    cliques = getsol(soltext,parmlist)
    if len(cliques)!=C:
        return False
    covered = {}
    subsets = list(itertools.combinations(list(range(1,N+1)),k))
    for s in subsets:
        covered[tuple(s)] = 0
    for c in cliques:
        if c[0]==0:
            subset = c[1:]
            if len(subset)!=k-1:
                return False
            for x in range(1,N+1):
                if x not in subset:
                    s = subset + [x]
                    s.sort()
                    covered[tuple(s)] = 1
        elif c[0]==1:
            subset = c[1:]
            if len(subset)!=k+1:
                return False
            for s in itertools.combinations(subset,k):
                t = list(s)
                t.sort()
                covered[tuple(t)] = 1
        else:
            return False
                                                 
    for s in subsets:
        if covered[tuple(s)]==0:
            return False
        
    return True                 
