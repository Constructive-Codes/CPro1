"""Use get_strategies() to prompt for solution strategies, then get_detailsprograms to prompt for details and generate candidate programs."""
import re
import json
import sys
import time
from multiprocessing import Pool
import utils
import problem_def
import conf
import candidate


def get_strategies(smallparams: list[int],openparams: list[int]) -> list[list[str]]:
    """Prompt for strategies, using example parameters smallparams and openparams.  Return list of strategies, each with label and summary text."""
    client = utils.connectmodel()
    prompt = f"{problem_def.PROB_DEF}  Please suggest {conf.NUM_STRATEGIES} different approaches we could implement in {conf.LANGUAGE}.  "
    prompt += f"For now, just describe the approaches.  Then I will pick one of the approaches, and you will write the {conf.LANGUAGE} code to test it.  We will start testing on small parameters like {utils.paramtext(problem_def.PARAMS,smallparams)}, and then once those work we will proceed to larger parameters like {utils.paramtext(problem_def.PARAMS,openparams)}.  Format your list items like this example: \"12. **Strategy Name**: Sentences describing strategy, all on one line...\""
    print(f"strategies prompt: {prompt}\n")
    m = utils.addmessage([],"user",prompt)
    result = utils.runprompt(m,conf.STRAT_TEMP,conf.TOP_P,client)
    print(f"strategies result: {result}\n")
    items = re.findall(r'^\s*\(?[0-9]+\)?[\)\.\:]\s*(?:\*+\"?|\")([^\*\"]+?)[\:\.]?(?:\"?\*+|\")\s*\.?\s*\:?\s*(.+?[a-z].*?)\s*$',result,flags=re.MULTILINE)
    for item in items:
        print(f"itemlabel |{item[0]}| itemtext |{item[1]}|")
    return items



def get_details(strat: str,strat_summary: str,jobnum: int) -> candidate.Candidate:
    """Prompt for strat details, before writing any code.  Return a newly-generated Candidate with the initial message sequence containing the details."""
    myclient = utils.connectmodel()
    c = candidate.Candidate()
    c.strat = strat
    c.strat_summary = strat_summary

    time.sleep(jobnum*conf.SEC_PER_REQ) # respect API limits on number of calls that can be initiated in a unit of time

    article = "a"
    if problem_def.FULL_PROB_NAME[0].lower() in 'aeiou':
        article = "an"
    detailsprompt = f"{problem_def.PROB_DEF}\n\nWe have selected the following approach"
    detailsprompt += f":\n{c.strat}: {c.strat_summary} Do not terminate until a valid solution is found.\n\nDescribe the elements of this approach to constructing {article} {problem_def.FULL_PROB_NAME}.  Do not yet write code; just describe the details of the approach."
    c.message_seq = utils.addmessage([],"user",detailsprompt)
    detailsresult = utils.runprompt(c.message_seq,conf.DETAILS_TEMP,conf.TOP_P,myclient)
    c.message_seq = utils.addmessage(c.message_seq,"assistant",detailsresult)

    return c


def get_programs(smallparams: list[int],openparams: list[int],c: candidate.Candidate,jobnum: int) -> candidate.Candidate:
    """Starting with the message sequence from get_details, prompt for the program and any hyperparameter ranges and add them to c."""
    myclient = utils.connectmodel()

    time.sleep(jobnum*conf.SEC_PER_REQ)

    programprompt = f"Now implement this approach in {conf.LANGUAGE}, following in detail the plan described above.  Provide the complete code.  The code should only print out the final {problem_def.FULL_PROB_NAME} once a valid solution is found.\n\n"
    programprompt += f"I will be running the code from the Linux command line.  Please have the {conf.LANGUAGE} code take command-line parameters: {" ".join(problem_def.PARAMS)} seed (in that order), followed by additional parameters as needed which represent hyperparameters of your approach.  The seed is the random seed (if no random seed is needed, still accept this parameter but ignore it).\n\nAfter giving the complete code"
    programprompt += ", for each hyperparameter that is an extra command-line parameter, provide a specification in JSON with fields \"name\",\"min\",\"max\",\"default\" specifying the name, minimum value, maximum value, and default value for the hyperparameter.  For example: {\"name\":\"gamma\", \"min\":0.0, \"max\":2.0, \"default\":0.5}.  If no hyperparameters are required then just state \"No Hyperparameters Required\""
    programprompt += f" after giving the complete code.  I will be using Linux timeout to set a time limit on execution of your program, and for challenging {problem_def.PROB_NAME} parameters this will be a long timeout (hours).  So to maximize chances of finding a solution your code should keep running indefinitely until it finds a valid solution.  Therefore, eliminate hyperparameters that would control termination, since your program needn't terminate until it succeeds.\n\nWe will start testing with small problem parameters like {utils.paramtext(problem_def.PARAMS,smallparams)}.  Once those work, we can then test further refinements and move towards larger problem parameters like {utils.paramtext(problem_def.PARAMS,openparams)}."
    c.message_seq = utils.addmessage(c.message_seq,"user",programprompt)
    programresult = utils.runprompt(c.message_seq,conf.PROG_TEMP,conf.TOP_P,myclient)
    c.message_seq = utils.addmessage(c.message_seq,"assistant",programresult)

    program = utils.extractprogram(programresult,conf.LANGUAGE)
    if program=="":
        hyperparams: list = []
    else:
        hyperparamlist = re.findall(r'```json\s*(.+?)\s*```',programresult,flags=re.MULTILINE|re.DOTALL)
        hyperparams = []
        for j in hyperparamlist:
            try:
                parsed_j = json.loads(j)
                if not isinstance(parsed_j,list):
                    parsed_j = [parsed_j]
                for h in parsed_j:
                    if isinstance(h['min'],(int,float)) and isinstance(h['max'],(int,float)) and isinstance(h['default'],(int,float)):
                        hyperparams = hyperparams + [[h['name'],h['min'],h['max'],h['default']]]
            except:
                # print("NOTICE: hyperparameter parsing exception")
                pass
    c.progtext = program
    c.parm_ranges = hyperparams
    return c


def get_detailsprograms(smallparams: list[int],openparams: list[int],strategies: list[list[str]]) -> list[candidate.Candidate]:
    """Starting from a list of strategies, get details and programs and hyperparameter ranges for each.  Return newly-created Candidates."""
    if conf.SEQUENTIAL:
        candidates = [get_details(strategies[i][0],strategies[i][1],i) for i in range(0,len(strategies))]
    else:
        detailsjobs = [(strategies[i][0],strategies[i][1],i) for i in range(0,len(strategies))]
        with Pool(processes=len(strategies)) as pool:
            candidates = pool.starmap(get_details,detailsjobs)

    if len(candidates)!=len(strategies):
        print(f"FATAL ERROR: candidate/strategies length mismatch {len(candidates)} {len(strategies)}")
        return []

    if conf.SEQUENTIAL:
        candidates = [get_programs(smallparams,openparams,candidates[i],i) for i in range(0,len(candidates))]
    else:
        programjobs = [(smallparams,openparams,candidates[i],i) for i in range(0,len(candidates))]
        with Pool(processes=len(strategies)) as pool:
            candidates = pool.starmap(get_programs,programjobs)

    for c in candidates:
        print(f"details prompt: {utils.getmessage(c.message_seq[0])}\n")
        print(f"details result: {utils.getmessage(c.message_seq[1])}\n")
        print(f"program prompt: {utils.getmessage(c.message_seq[2])}\n")
        print(f"program result: {utils.getmessage(c.message_seq[3])}\n")
        if c.progtext=="":
            print("NOTICE: program not found")
        else:
            print(f"program:\n{c.progtext}\n")
        print(f"hyperparams: {c.parm_ranges}\n")
        sys.stdout.flush()

    return candidates
