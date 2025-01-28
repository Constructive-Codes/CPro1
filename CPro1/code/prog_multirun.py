"""Provides multirun() which compiles generated C code and runs it in parallel in firejail."""
from pathlib import Path
import subprocess
import pickle
import utils
import conf

def multirun(runlabel: str,runnum: int,progtext: str,subprogram: str,safetylimit: float,passthroughs: list) -> list|None:
    """Compile C program progtext into tmpdir...runnum, then use subprogram (hyper_sub_multi.py or multirun.py) to run compiled program in firejail."""
    progname = utils.setupcprog(runlabel,runnum,progtext)
    if progname=="": # immediate failure
        return None
    for p in [subprogram,"utils.py","conf.py","multirun.py"]:
        subprocess.run(f"cp ./{p} ./tmpdir{progname}",shell=True,check=True)
    with open(f"./tmpdir{progname}/parms.pkl","wb") as g:
        pickle.dump([progname] + passthroughs,g)
    result = subprocess.run(f"timeout {safetylimit} firejail --net=none --seccomp --noroot --caps.drop=all --rlimit-as={conf.MEM_LIMIT}g --private=./tmpdir{progname} python3 {subprogram}",shell=True,capture_output=True,text=True)
    logtext = result.stdout
    errtext = result.stderr
    print(logtext + "\n")
    print(errtext + "\n")
    fname = f"./tmpdir{progname}/results.pkl"
    if Path(fname).exists():
        with open(fname,"rb") as f:
            res = pickle.load(f)
            return [res,logtext]
    else:
        return None
