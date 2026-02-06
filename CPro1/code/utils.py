"""Utility functions."""
import re
import subprocess
import sys
import random
from pathlib import Path
from typing import Any # to support clients from various providers
import time
import conf


def connectmodel() -> Any:
    """Return a client from conf.PROVIDER using conf.API_KEY."""
    if conf.PROVIDER=="OPENAI":
        from openai import OpenAI
        return OpenAI(api_key=conf.API_KEY)
    if conf.PROVIDER=="MISTRAL":
        from mistralai import Mistral
        return Mistral(api_key=conf.API_KEY)
    if conf.PROVIDER=="ANTHROPIC":
        import anthropic
        return anthropic.Anthropic(api_key=conf.API_KEY)
    if conf.PROVIDER=="DEEPSEEK":
        from openai import OpenAI
        return OpenAI(api_key=conf.API_KEY,base_url="https://api.deepseek.com",timeout=conf.TIMEOUT)
    if conf.PROVIDER=="DEEPINFRA":
        from openai import OpenAI
        return OpenAI(api_key=conf.API_KEY,base_url="https://api.deepinfra.com/v1/openai")
    if conf.PROVIDER=="OLLAMA":
        from openai import OpenAI
        return OpenAI(api_key=conf.API_KEY,base_url="http://localhost:11434/v1",timeout=conf.TIMEOUT)        
    return None


def addmessage(m: list[dict],role: str,newmsg: str) -> list[dict]:
    """Append newmsg with role to the sequence of messages m, using the format required by conf.PROVIDER."""
    if conf.PROVIDER in ("OPENAI","ANTHROPIC"):
        if conf.PROVIDER=="OPENAI" and m==[] and conf.REASONING:
            m = m + [{"role": "developer", "content": [{"type": "text", "text": "Formatting re-enabled"}]}]
        return m + [{"role": role, "content": [{"type": "text", "text": newmsg}]}]
    # otherwise, conf.PROVIDER=="MISTRAL" or conf.PROVIDER=="DEEPSEEK" or conf.PROVIDER=="DEEPINFRA":
    return m + [{"role": role, "content": newmsg}]


def getmessage(m: dict) -> str:
    """Given a dict from the message sequence formatted according to conf.PROVIDER, return the text of the message."""
    if conf.PROVIDER in ("OPENAI","ANTHROPIC"):
        return m["content"][0]["text"]
    # otherwise, conf.PROVIDER=="MISTRAL" or conf.PROVIDER=="DEEPSEEK" or conf.PROVIDER=="DEEPINFRA":
    return m["content"]


def runprompt(m: list[dict],t: float,tp: float,client: Any) -> str:
    """Given message sequence m as a prompt, return result from client using conf.PROVIDER.  t is temperature, tp is top_p (if applicable)."""
    retrywait = conf.INITIAL_RETRY_WAIT
    numretries = 0
    while numretries<conf.MAX_RETRIES:
        try:
            if conf.PROVIDER=="OPENAI":
                if conf.REASONING:
                    if conf.FLEX:
                        response = client.chat.completions.create(model=conf.MODEL,reasoning_effort=conf.REASONING_EFFORT,messages=m,service_tier="flex",timeout=conf.FLEX_TIMEOUT) # flex service tier
                    else:
                        response = client.chat.completions.create(model=conf.MODEL,reasoning_effort=conf.REASONING_EFFORT,messages=m) # no max_tokens - for supporting reasoning models (with effort "high")
                else:
                    response = client.chat.completions.create(model=conf.MODEL,messages=m,temperature=t,max_tokens=conf.MAX_TOKENS,top_p=tp,frequency_penalty=0,presence_penalty=0)
                return response.choices[0].message.content
            if conf.PROVIDER=="MISTRAL":
                response = client.chat.complete(model=conf.MODEL,messages=m,temperature=t,max_tokens=conf.MAX_TOKENS)
                return response.choices[0].message.content
            if conf.PROVIDER=="ANTHROPIC":
                response = client.messages.create(model=conf.MODEL,messages=m,temperature=t,max_tokens=conf.MAX_TOKENS)
                return response.content[0].text
            if conf.PROVIDER=="DEEPSEEK":
                if conf.REASONING:
                    response = client.chat.completions.create(model=conf.MODEL,messages=m,temperature=t)
                else:
                    response = client.chat.completions.create(model=conf.MODEL,messages=m,temperature=t,max_tokens=conf.MAX_TOKENS)
                return response.choices[0].message.content
            if conf.PROVIDER=="DEEPINFRA":
                response = client.chat.completions.create(model=conf.MODEL,messages=m,temperature=t,max_tokens=conf.MAX_TOKENS,stream=False)
                return response.choices[0].message.content
            if conf.PROVIDER=="OLLAMA":
                if conf.REASONING:
                    response = client.chat.completions.create(model=conf.MODEL,messages=m) # use Ollama default temperature etc.
                    responsestring = response.choices[0].message.content
                    return responsestring.split("</think>")[-1].lstrip()  # remove reasoning portion of the content
                else:
                    response = client.chat.completions.create(model=conf.MODEL,messages=m,temperature=t,max_tokens=conf.MAX_TOKENS)
                    return response.choices[0].message.content                
        except Exception as e:
            print(f"NOTICE: API FAILURE - retrying after {retrywait} seconds - exception: {e}")
            sys.stdout.flush()
            time.sleep(random.randint(retrywait//2,retrywait))
            retrywait = int(retrywait * conf.RETRY_WAIT_MULTIPLIER)
            numretries += 1
    print("ERROR: API FAILURE - no more retries")
    sys.stdout.flush()
    return "" # failure


def paramtext(params: list[str],values: list[int]) -> str:
    """Given named params with values, return a string with their assignments."""
    s = ""
    for i, parm in enumerate(params):
        s += f"{parm}={values[i]}"
        if i<len(params)-1:
            s += " "
    return s

def hyperparamtext(h: list[list],values: list[float]) -> str:
    """Given hyperparam list h and values, return a string with their assignments."""
    s = ""
    for i,hyp in enumerate(h):
        s += f"{hyp[0]}={values[i]}"
        if i<len(h)-1:
            s += ", "
    return s


def timingtext(timings: list[list],time_limit: float,paramnames: list[str]) -> str:
    """Given paramnames and instance results in timings with time_limit, return a prompt substring describing the results."""
    s = ""
    for j,t in enumerate(timings):
        if j>0:
            s += ", "
        if j==len(timings)-1:
            s += "and "
        parmstring = paramtext(paramnames,t[0].split())
        attempts = len(t[1])
        solves = sum(1 for ti in t[1] if ti >= 0)
        tottime = sum(ti for ti in t[1] if ti >= 0)
        if solves==0:
            s += f"fails to solve {parmstring} in {attempts} attempts with a time limit of {time_limit} seconds each"
        elif solves<attempts:
            s += f"solves {parmstring} in {solves} out of {attempts} attempts with a time limit of {time_limit} seconds each"
        else:
            s += f"solves {parmstring} in an average of {round(tottime/attempts,4)} seconds across {attempts} attempts"
    return s


def extractprogram(result: str,language: str) -> str:
    """Given response result that may contain a program in language, use its formatting to extract the program."""
    programs = re.findall(r'```' + language.lower() + r'\s*(.+?)\s*```',result,flags=re.MULTILINE|re.DOTALL)
    if len(programs)==0:
        program = ""
        if conf.LANGUAGE=="C": # even without the Markdown formatting, programs fairly reliable start with #include and end with } at start of line
            programs = re.findall(r'(\#include.*^\}\s*)',result,flags=re.MULTILINE|re.DOTALL)
            if len(programs)>0 and len(programs[0])>10:
                program = programs[0]
    else:
        program = programs[-1] # take last program.  If there are multiple, it is occasionally due to a revision in the response, so we want the last one.
    return program


def setupcprog(runlabel: str,runnum: int,progtext: str) -> str:
    """Put progtext into a .c file and compile it, in a tmpdir.  Use runlabel and runnum to name .c and tmpdir, and return resulting name."""
    progname = f"prog{runlabel}r{runnum}"
    with open(f"{progname}.c","w",encoding="UTF-8") as f:
        f.write(progtext)
    if Path(f"./tmpdir{progname}").exists():
        subprocess.run(f"rm -r ./tmpdir{progname}",shell=True,check=True)
    subprocess.run(f"mkdir ./tmpdir{progname}",shell=True,check=True)
    for p in [f"{progname}.c","utils.py","problem_def.py"]:
        subprocess.run(f"cp ./{p} ./tmpdir{progname}",shell=True,check=True)
    subprocess.run(f"gcc -march=native -O3 -o ./tmpdir{progname}/{progname} ./tmpdir{progname}/{progname}.c -lm",shell=True)
    if not Path(f"./tmpdir{progname}/{progname}").exists():
        return ""
    return progname


def instanceresultlist(b:list[list]) -> list[list]:
    """Given raw result list b with unordered instances, return a reformatted list of results grouped by instance."""
    resultlist = []
    for inst in list(dict.fromkeys([f[0] for f in b])):
        timinglist = []
        for inst2 in [[f[0],f[4]] for f in b]:
            if inst==inst2[0]:
                timinglist += [inst2[1]]
        resultlist += [[inst,timinglist]]
    return resultlist


def parsesoltext(soltext: str,n: int) -> list[list[int]]:
    """Given program output, extract a 2-d solution array from the last n lines that have solution-like format."""
    array: list[list[int]] = []
    lines = soltext.splitlines()
    for line in lines:
        rows = re.findall(r'((?:\-?\d+[, ]+){1,}\-?\d+)',line)
        if len(rows)>0:
            row = rows[0]
            elems = re.findall(r'\-?\d+',row)
            array = array + [[int(e) for e in elems]]
    return array[(-n):]


def prettystring(x: list|int) -> str:
    """Recursive pretty printer for solutions."""
    if not isinstance(x,list):
        return str(x)
    if all(not isinstance(item,list) for item in x):
        return '\t'.join(str(item) for item in x)
    return '\n'.join([prettystring(item) for item in x]) + '\n'


def computescore(res: list[list]) -> float:
    """Given results and timings in res, return score: #solved problems + a timing bonus that is between 0 and 1."""
    totscore = sum(r[2] for r in res)
    if totscore>0:
        totscore += (sum(r[3]*r[2] for r in res)/sum(r[2] for r in res)) # #solved problems + average time bonus for solved problems
    return totscore
