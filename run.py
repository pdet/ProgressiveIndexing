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
for column_size in COLUMN_SIZE_LIST:
    for query in ALL_WORKLOAD_LIST:
        q_path = query_path(experiment_path,QUERY_SELECTIVITY,query)
        a_path = answer_path(experiment_path,QUERY_SELECTIVITY,query)
        generate_query(NUM_QUERIES,column_size,experiment_path,q_path,a_path,QUERY_SELECTIVITY,query)