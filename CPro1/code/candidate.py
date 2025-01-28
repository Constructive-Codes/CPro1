"""Candidate program, with values getting filled in as the orchestrator proceeds through its stages."""

class Candidate:
    def __init__(self):
        self.strat: str = "" # strategy name
        self.strat_summary: str = "" # brief strategy description
        self.progtext: str = "" # current program
        self.parm_ranges: list[list] = [] # range [name,min,max,default] for each hyperparameter (empty list if none)
        self.message_seq: list[dict] = [] # sequence of prompts and replies that generated the program
        self.score: float = 0.0 # evaluation score: number of solved instances, plus a time bonus between 0 and 1
        self.hyperparams: list[float] = [] # tuned hyperparameter values
        self.time_limit: float = 0.0 # time limit under which score was obtained
        self.instance_timings: list[list] = [] # timings on each instance, for the best selected hyperparameters
        self.full_dev_score: float = 0.0  # score using conf.FULL_DEV_TIME, if this candidate is selected for running on the full dev set
        self.full_dev_instance_timings: list[list] = [] # instance timings using conf.FULL_DEV_TIME
        self.open_problem_score: float = 0.0  # score on open problems using conf.FULL_OPEN_TIME, if this candidate is selected for running on the open problems
        self.open_problem_results: list[list] = [] # results from open problems using conf.FULL_OPEN_TIME
