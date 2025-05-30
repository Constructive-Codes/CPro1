PROB_NAME = "CS"
FULL_PROB_NAME = "Cap Set"
PROB_DEF = "A \"Cap Set\" CS(n,s) is a subset S of Z_3^{n}, such that S has at least s distinct points, and no three points {x,y,z} in S satisfy x+y+z=0 (vector addition over Z_3^{n}, so addition is taken modulo 3).  We represent the cap set by an array with at least s rows, and n columns in each row.  Elements of the array are from Z_3={0,1,2}, and each row represents a point in Z_3^{n} which is an element of S.  Given (n,s) we want to construct a Cap Set CS(n,s)."

PARAMS = ["n","s"]
DEV_INSTANCES = [[3,9],[4,20],[5,45],[6,112],[7,236],[8,496]] # ,[10,2240]]
OPEN_INSTANCES = [[7,237],[8,512]] # ,[9,1065],[10,2241]]

import utils

def getsol(soltext: str,parmlist: list[int]) -> list:
    """Extract solution array from raw program output soltext."""
    # utils.parsesoltext will do the work; just need to specify how many rows (how many lines of text) the solution array will have
    s = parmlist[1]
    array = utils.parsesoltext(soltext,s)
    return array 

def v(soltext: str,parmlist: list[int]) -> bool:
    """Verify that the solution in raw program output soltext is a valid solution with parameters in parmlist."""
    n = parmlist[0]
    s = parmlist[1]
    array = getsol(soltext,parmlist)
    if len(array)!=s:
        return False
    occur = {}
    for r in array:
        if len(r)!=n:
            return False
        for x in r:
            if x!=0 and x!=1 and x!=2:
                return False
        t = tuple(r)
        if t in occur:
            return False # duplicate
        occur[t] = 1
    for i in range(0,s):
        for j in range(i+1,s):
            needed = [(6-(array[i][k]+array[j][k]))%3 for k in range(0,n)]
            if tuple(needed) in occur: # 3 points sum to 0
                return False
    return True                 



