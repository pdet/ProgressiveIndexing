import os
import inspect
import sqlite3

db = sqlite3.connect('results.db')
cursor = db.cursor()

SCRIPT_PATH =  os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe()))) # script directory
os.chdir(SCRIPT_PATH)
os.chdir("..")

# Setting Values For Different Workloads
SkyServer=1
Random=2
SeqOver=3
SeqRand=4
ZoomIn=5
SeqZoomIn=6
Skew=7
ZoomOutAlt=8
Periodic=9
ZoomInAlt=10

#Setting Values For Different Algorithms
FullScan = 1
FullIndex = 2
StandardCracking = 3
StochasticCracking = 4
ProgressiveStochasticCracking=5
CoarseGranularIndex=6
ProgressiveQuicksort=7
ProgressiveQuicksortCostModel=8
ProgressiveBucketsortEquiheight=9
ProgressiveBucketsortEquiheightCostModel=10
ProgressiveRadixsortLSD=11
ProgressiveRadixsortLSDCostModel=12
ProgressiveRadixsortMSD=13
ProgressiveRadixsortMSDCostModel=14
AdaptiveAdaptiveIndexing=15
ProgressiveRadixsortMSDPure=16
ProgressiveRadixsortMSDPureCostModel=17

def column_path(COLUMN_SIZE):
    path = "generated_data/" +str(COLUMN_SIZE)
    os.system('mkdir -p '+ path)
    return path+ "/"


def query_path(EXPERIMENT_PATH, SELECTIVITY_PERCENTAGE,QUERIES_PATTERN):
    return EXPERIMENT_PATH + "query_" + str(SELECTIVITY_PERCENTAGE) + "_" + str(QUERIES_PATTERN)

def answer_path(EXPERIMENT_PATH, SELECTIVITY_PERCENTAGE,QUERIES_PATTERN):
    return EXPERIMENT_PATH + "answer_" + str(SELECTIVITY_PERCENTAGE) + "_" + str(QUERIES_PATTERN)

def generate_column(column_size):
    COLUMN_PATH = column_path(column_size)+'column'
    print("Generating Column")
    codestr = "./generate_column --column-size=" + str(column_size) + " --column-path=" + str(COLUMN_PATH)
    print (codestr)
    os.system(codestr)

def generate_workload(num_queries,column_size,query,selectivity):
    COLUMN_PATH = column_path(column_size)
    QUERY_PATH = query_path(COLUMN_PATH,selectivity,query)
    ANSWER_PATH = answer_path(COLUMN_PATH,selectivity,query)
    print("Generating Queries")
    codestr = "./generate_workload --num-queries=" + str(num_queries) + " --column-size=" + str(column_size)  \
              + " --column-path=" + str(COLUMN_PATH)+ "column" + " --query-path=" + str(QUERY_PATH) + " --answer-path=" + str(ANSWER_PATH) + " --selectivity=" \
              + str(selectivity) + " --queries-pattern=" +  str(query)
    print (codestr)
    os.system(codestr)

def clean_generated_data():
    os.system ('rm -r generated_data')

def compile():
    print("Compiling")
    os.environ['OPT'] = 'true'
    if os.system('cmake -DCMAKE_BUILD_TYPE=Release && make') != 0:
        print("Make Failed")
        exit()

def generate_cost_model():
    print("Generating Cost Model Constants")
    os.system("python scripts/cost_model/generate_constants.py")  


def run_experiment(COLUMN_SIZE,QUERY_PATTERN,QUERY_SELECTIVITY,ALGORITHM,NUM_QUERIES):
    COLUMN_PATH = column_path(COLUMN_SIZE)
    QUERY_PATH = query_path(COLUMN_PATH,QUERY_SELECTIVITY,QUERY_PATTERN)
    ANSWER_PATH = answer_path(COLUMN_PATH,QUERY_SELECTIVITY,QUERY_PATTERN)
    codestr ="./main --num-queries=" + str(NUM_QUERIES) + " --column-size=" + str(COLUMN_SIZE) + \
             " --algorithm="+str(ALGORITHM)+ " --column-path=" + str(COLUMN_PATH + "column") + " --query-path=" \
             + str(QUERY_PATH) + " --answer-path=" + str(ANSWER_PATH) + " --correctness=" + str(1)
    print(codestr)
    if os.system(codestr) != 0:
        print("Failed!")

def template_correctness():
    ALGORITHM_LIST = [17] #list(range(1,18))
    # ALGORITHM_LIST = [ProgressiveQuicksortCostModel]
    COLUMN_SIZE_LIST = [1000000]
    syntethical_workload_list = [Random]
    # syntethical_workload_list = [2,3]
    QUERY_SELECTIVITY_LIST = [0.1]
    num_queries = 1000
    clean_generated_data()
    generate_cost_model()
    compile()
    for column_size in COLUMN_SIZE_LIST:
        generate_column(column_size)
        for query in syntethical_workload_list:
            for selectivity in QUERY_SELECTIVITY_LIST:
                generate_workload(num_queries,column_size,query,selectivity)
                for algorithm in ALGORITHM_LIST:
                    run_experiment(column_size,query,selectivity,algorithm,num_queries)


template_correctness()

