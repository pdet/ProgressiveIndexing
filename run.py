import os
import inspect

SCRIPT_PATH =  os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe()))) # script directory
os.chdir(SCRIPT_PATH)

# Setting Values For Column Distributions
RANDOM = "1"

# Setting Values For Different Workloads
SkyServer=0
Random=1
SeqOver=2
SeqInv=3
SeqRand=4
SeqNoOver=5
SeqAlt=6
ConsRandom=7
ZoomIn=8
ZoomOut=9
SeqZoomIn=10
SeqZoomOut=11
Skew=12
ZoomOutAlt=13
SkewZoomOutAlt=14
Periodic=15
Mixed=16

#Setting Values For Different Algorithms
FullScan = 0
FullIndex = 1
StandardCracking = 2
StochasticCracking = 3
ProgressiveStochasticCracking=4
CoarseGranularIndex=5
ProgressiveQuicksort=6
ProgressiveQuicksortCostModel=7

#Setting Values For Update Algorithms
MergeComplete = 8
MergeGradually = 9
MergeRipple = 10
ProgressiveMergesort = 11

COLUMN_SIZE_LIST = [100000000]#[100000000,1000000000]
SWAP_PG_CRACKING_LIST = [0.1]
ALL_WORKLOAD_LIST = [Random,SeqOver,SeqInv,SeqRand,SeqNoOver,SeqAlt,ConsRandom,ZoomIn,ZoomOut,SeqZoomIn,SeqZoomOut,Skew,
                     ZoomOutAlt,SkewZoomOutAlt,Periodic,Mixed]

NUM_QUERIES = 1000
QUERY_SELECTIVITY = 0.01

PATH = ''

def translate_alg(alg):
    if alg == FullScan:
        return 'fs'
    if alg == StandardCracking:
        return 'std'
    if alg == StochasticCracking:
        return 'stc'
    if alg == CoarseGranularIndex:
        return 'cgi'
    if alg == ProgressiveStochasticCracking:
        return 'pstc'
    if alg == FullIndex:
        return 'fi'
    if alg == ProgressiveQuicksort:
        return 'pqs'
    if alg == ProgressiveQuicksortCostModel:
        return 'pqscm'
    return alg

print("Generating Cost Model Constants")
os.system("python scripts/cost_model/generate_constants.py")

print("Compiling")
os.environ['OPT'] = 'true'
if os.system('cmake -DCMAKE_BUILD_TYPE=Release && make') != 0:
    print("Make Failed")
    exit()


def column_path(COLUMN_SIZE):
    path = "generated_data/" +str(COLUMN_SIZE)
    os.system('mkdir -p '+ path)
    return path+ "/"

def generate_column(COLUMN_SIZE,COLUMN_PATH):
    print("Generating Column")
    codestr = "./generate_column --column-size=" + str(COLUMN_SIZE) + " --column-path=" + str(COLUMN_PATH)
    print (codestr)
    if os.system(codestr) != 0:
        print("Generating Column Failed")
        exit()

# Uniform Random Column Distribution
for column_size in COLUMN_SIZE_LIST:
    experiment_path = column_path(column_size)
    generate_column(column_size,experiment_path + "column")

def query_path(EXPERIMENT_PATH, SELECTIVITY_PERCENTAGE,QUERIES_PATTERN):
    return EXPERIMENT_PATH + "query_" + str(SELECTIVITY_PERCENTAGE) + "_" + str(QUERIES_PATTERN)

def answer_path(EXPERIMENT_PATH, SELECTIVITY_PERCENTAGE,QUERIES_PATTERN):
    return EXPERIMENT_PATH + "answer_" + str(SELECTIVITY_PERCENTAGE) + "_" + str(QUERIES_PATTERN)

def generate_query(NUM_QUERIES,COLUMN_SIZE, COLUMN_PATH, QUERY_PATH,ANSWER_PATH, SELECTIVITY_PERCENTAGE,QUERIES_PATTERN):
    print("Generating Queries")
    codestr = "./generate_workload --num-queries=" + str(NUM_QUERIES) + " --column-size=" + str(COLUMN_SIZE)  \
              + " --column-path=" + str(COLUMN_PATH)+ "column" + " --query-path=" + str(QUERY_PATH) + " --answer-path=" + str(ANSWER_PATH) + " --selectivity=" \
              + str(SELECTIVITY_PERCENTAGE) + " --queries-pattern=" +  str(QUERIES_PATTERN)

    print (codestr)
    if os.system(codestr) != 0:
        print("Generating Queries Failed")
        exit()

def getFolderToSaveExperiments(algorithm,folder=""):
    global PATH
    if os.path.exists("ResultsCSV/+folder") != 1:
        os.system('mkdir -p ResultsCSV/'+folder)
    experimentsList = os.listdir("ResultsCSV/"+folder)
    aux = 0
    for experiment in experimentsList:
        if experiment != ".DS_Store":
            if aux < int(experiment):
                aux = int(experiment)
    currentexperiment = aux + 1
    PATH = "ResultsCSV/"+ folder+translate_alg(algorithm) + "/"+ str(currentexperiment) + '/'
    os.system('mkdir -p ' + PATH)

#Output is a csv file with:
# ""algorithm;query_pattern;query_number;delta;query_time"
def generate_output(file,query_result,QUERY_PATTERN,algorithm):
    query_result = query_result.split("\n")
    for query in range(0, len(query_result)/2 -1):
        file.write(translate_alg(algorithm) + ";" + str(query) + ";" + str(QUERY_PATTERN) + ';' + query_result[query])
        file.write('\n')
    file.close()

# Saving Experiments
def create_output():
    header = "algorithm;query_pattern;query_number;delta;query_time"
    file = open(PATH + "results.csv", "w")
    file.write(header)
    file.write('\n')
    return file

#Generate Query Patterns
# for column_size in COLUMN_SIZE_LIST:
#     for query in ALL_WORKLOAD_LIST:
#         q_path = query_path(experiment_path,QUERY_SELECTIVITY,query)
#         a_path = answer_path(experiment_path,QUERY_SELECTIVITY,query)
#         generate_query(NUM_QUERIES,column_size,experiment_path,q_path,a_path,QUERY_SELECTIVITY,query)

def run_experiment(COLUMN_SIZE,QUERY_PATTERN,QUERY_SELECTIVITY,ALGORITHM,CORRECTNESS=0):
    COLUMN_PATH = column_path(COLUMN_SIZE)
    QUERY_PATH = query_path(COLUMN_PATH,QUERY_SELECTIVITY,QUERY_PATTERN)
    ANSWER_PATH = answer_path(COLUMN_PATH,QUERY_SELECTIVITY,QUERY_PATTERN)
    codestr ="./main --num-queries=" + str(NUM_QUERIES) + " --column-size=" + str(COLUMN_SIZE) + \
             " --algorithm="+str(ALGORITHM)+ " --column-path=" + str(COLUMN_PATH + "column") + " --query-path=" \
             + str(QUERY_PATH) + " --answer-path=" + str(ANSWER_PATH) + " --correctness=" + str(CORRECTNESS)
    print(codestr)
    if CORRECTNESS:
        if os.system(codestr) != 0:
            print("Failed!")
    else:
        getFolderToSaveExperiments(ALGORITHM,str(COLUMN_SIZE)+"_"+str(QUERY_PATTERN)+ "/")
        result = os.popen(codestr).read()
        file = create_output()
        generate_output(file,result,QUERY_PATTERN,ALGORITHM)

def run_all_workloads(ALGORITHM,CORRECTNESS=0):
    # ALL_WORKLOAD_LIST = [Random]
    for column_size in COLUMN_SIZE_LIST:
        for query in ALL_WORKLOAD_LIST:
            run_experiment(column_size,query,QUERY_SELECTIVITY,ALGORITHM,CORRECTNESS)


def test_correctness():
    ALGORITHM_LIST = [FullScan,FullIndex,StandardCracking,StochasticCracking,ProgressiveStochasticCracking,CoarseGranularIndex,ProgressiveQuicksort,ProgressiveQuicksortCostModel]
    # ALGORITHM_LIST = []
    for algorithm in ALGORITHM_LIST:
        run_all_workloads(algorithm,1)



def run():
    ALGORITHM_LIST = [FullScan,FullIndex,StandardCracking,StochasticCracking,ProgressiveStochasticCracking,CoarseGranularIndex,ProgressiveQuicksort,ProgressiveQuicksortCostModel]
    # ALGORITHM_LIST = [ProgressiveQuicksortCostModel]
    for algorithm in ALGORITHM_LIST:
        run_all_workloads(algorithm)

def test_correctness_updates():
    ALGORITHM_LIST = [MergeComplete,MergeGradually,MergeRipple,ProgressiveMergesort]
    # ALGORITHM_LIST = [ProgressiveQuicksortCostModel]
    for algorithm in ALGORITHM_LIST:
        for column_size in COLUMN_SIZE_LIST:
            run_experiment(column_size,Random,QUERY_SELECTIVITY,algorithm,1)

def run_updates():
    ALGORITHM_LIST = [MergeComplete,MergeGradually,MergeRipple,ProgressiveMergesort]
    # ALGORITHM_LIST = [ProgressiveQuicksortCostModel]
    for algorithm in ALGORITHM_LIST:
        for column_size in COLUMN_SIZE_LIST:
            run_experiment(column_size,Random,QUERY_SELECTIVITY,algorithm,0)

# test_correctness()
test_correctness_updates()
# run()