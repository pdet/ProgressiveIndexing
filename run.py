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
COLUMN_SIZE_LIST = [100000000]#[100000000,1000000000]
ALL_WORKLOAD_LIST = [Random,SeqOver,SeqInv,SeqRand,SeqNoOver,SeqAlt,ConsRandom,ZoomIn,ZoomOut,SeqZoomIn,SeqZoomOut,Skew,
                     ZoomOutAlt,SkewZoomOutAlt,Periodic,Mixed]
NUM_QUERIES = 1000
QUERY_SELECTIVITY = 0.01
NUMBER_OF_REPETITIONS = 10

PATH = ''


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

#Generate Query Patterns
# for column_size in COLUMN_SIZE_LIST:
#     for query in ALL_WORKLOAD_LIST:
#         q_path = query_path(experiment_path,QUERY_SELECTIVITY,query)
#         a_path = answer_path(experiment_path,QUERY_SELECTIVITY,query)
#         generate_query(NUM_QUERIES,column_size,experiment_path,q_path,a_path,QUERY_SELECTIVITY,query)

def run_experiment(COLUMN_SIZE,QUERY_PATTERN,QUERY_SELECTIVITY,ALGORITHM,CORRECTNESS=True):
    COLUMN_PATH = column_path(COLUMN_SIZE)
    QUERY_PATH = query_path(COLUMN_PATH,QUERY_SELECTIVITY,QUERY_PATTERN)
    ANSWER_PATH = answer_path(COLUMN_PATH,QUERY_SELECTIVITY,QUERY_PATTERN)
    codestr ="./main --num-queries=" + str(NUM_QUERIES) + " --column-size=" + str(COLUMN_SIZE) + \
             " --algorithm="+str(ALGORITHM)+ " --column-path=" + str(COLUMN_PATH + "column") + " --query-path=" \
             + str(QUERY_PATH) + " --answer-path=" + str(ANSWER_PATH)
    print(codestr)
    if CORRECTNESS:
        if os.system(codestr) != 0:
            print("Failed!")
    else:
        result = os.popen(codestr).read()
        print(result)
    # else:
    #     getFolderToSaveExperiments(COLUMN_PATTERN+"_"+QUERY_PATTERN+ "/")
    #     repetition =1
    #     result = os.popen(codestr).read()
    #     file = create_output()
    #     generate_output(file,result,repetition,QUERY_PATTERN,COLUMN_PATTERN,PIVOT_TYPE,PIVOT_SELECTION_TYPE,PIECE_TO_CRACK_TYPE)

def run_all_workloads(ALGORITHM,CORRECTNESS=True):
    for column_size in COLUMN_SIZE_LIST:
        for query in ALL_WORKLOAD_LIST:
            run_experiment(column_size,query,QUERY_SELECTIVITY,ALGORITHM,CORRECTNESS)


def test_correctness():
    ALGORITHM_LIST = [FullScan,FullIndex]
    for algorithm in ALGORITHM_LIST:
        run_all_workloads(algorithm)

test_correctness()


# def run():
#     PIVOT_TYPES_LIST = [PIVOT_EXACT_PREDICATE,PIVOT_WITHIN_QUERY_PREDICATE,PIVOT_WITHIN_QUERY,PIVOT_WITHIN_COLUMN]
#     PIVOT_SELECTION_LIST = [RANDOM_P,MEDIAN,APPROXIMATE_MEDIAN]
#     PIECE_TO_CRACK_LIST = [ANY_PIECE,BIGGEST_PIECE]
#     for pivot_type in PIVOT_TYPES_LIST:
#         if pivot_type == PIVOT_EXACT_PREDICATE:
#             run_all_workloads(pivot_type,CORRECTNESS = False)
#         if pivot_type == PIVOT_WITHIN_QUERY_PREDICATE:
#             for pivot_selection in PIVOT_SELECTION_LIST:
#                 run_all_workloads(pivot_type,pivot_selection,CORRECTNESS = False)
#         if pivot_type == PIVOT_WITHIN_QUERY:
#             for pivot_selection in PIVOT_SELECTION_LIST:
#                 for piece_to_crack in PIECE_TO_CRACK_LIST:
#                     run_all_workloads(pivot_type,pivot_selection,piece_to_crack,CORRECTNESS = False)
#         if pivot_type == PIVOT_WITHIN_COLUMN:
#             for pivot_selection in PIVOT_SELECTION_LIST:
#                 for piece_to_crack in PIECE_TO_CRACK_LIST:
#                     run_all_workloads(pivot_type,pivot_selection,piece_to_crack,CORRECTNESS = False)
#
# run()