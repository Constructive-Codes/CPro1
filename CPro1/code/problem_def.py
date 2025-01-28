"""Symmetric Weighing Matrix definitions."""

PROB_NAME = "SymmW" # Short abbreviated name for problem
FULL_PROB_NAME = "Symmetric Weighing Matrix" # Full name for problem in Title Caps
PROB_DEF = "A \"weighing matrix\" W(n,w) with parameters (n,w) is an n by n square matrix (n rows and n columns) with entries in {0,1,-1} that satisfies W W^T = wI.  That is, W times its transpose is equal to the constant w times the identity matrix I.  The weighing matrix will have w nonzero entries in each row and each column.  And each pair of distinct rows is orthogonal (dot product zero).  Given (n,w), we want to construct \"SymmW\", a symmetric weighing matrix W(n,w) that satisfies these properties and is also a symmetric matrix."
PARAMS = ["n","w"] # parameter names
DEV_INSTANCES = [[7,4], [10,4], [10,9], [14,9], [16,9], [17,9], [18,16], [21,16]] # PARAMS for each dev instance.  Note this must be <= conf.NUM_PROCESSES
OPEN_INSTANCES = [[19,9], [21,9], [22,16], [23,16]] # PARAMS for each open instance.  Must be <= conf.NUM_PROCESSES.

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
    w = parmlist[1]
    W = getsol(soltext,parmlist)

    if len(W) != n or any(len(row) != n for row in W):
        return False
    
    # Check if all entries in W are in {0, 1, -1}
    if not all(entry in {0, 1, -1} for row in W for entry in row):
        return False
    
    # Check if W is symmetric
    for i in range(n):
        for j in range(n):
            if W[i][j] != W[j][i]:
                return False
    
    # Calculate W * W^T
    WWT = [[sum(W[i][k] * W[j][k] for k in range(n)) for j in range(n)] for i in range(n)]
    
    # Check if W * W^T == w * I
    for i in range(n):
        for j in range(n):
            if i == j:
                if WWT[i][j] != w:
                    return False
            else:
                if WWT[i][j] != 0:
                    return False
    
    # Check if each row and column has exactly w nonzero entries
    for i in range(n):
        if sum(1 for x in W[i] if x != 0) != w:
            return False
        if sum(1 for x in [W[j][i] for j in range(n)] if x != 0) != w:
            return False
    
    return True

    

