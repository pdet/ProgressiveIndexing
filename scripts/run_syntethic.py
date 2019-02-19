import os
import inspect
import sqlite3

os.system("rm results.db")
os.system("python scripts/sqlite.py")
db = sqlite3.connect('results.db')
cursor = db.cursor()

SCRIPT_PATH =  os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe()))) # script directory
os.chdir(SCRIPT_PATH)
os.chdir("..")

# Setting Values For Different Workloads
SkyServer=1
Random=2
SeqOver=3
SeqRand=3
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

baseline_list = [FullScan,FullIndex,StandardCracking,StochasticCracking,ProgressiveStochasticCracking,CoarseGranularIndex]
progressive_list = [ProgressiveQuicksort,ProgressiveRadixsortMSD, ProgressiveRadixsortLSD, ProgressiveBucketsortEquiheight]
progressive_cm_list = [ProgressiveQuicksortCostModel,ProgressiveRadixsortMSDCostModel, ProgressiveRadixsortLSDCostModel, ProgressiveBucketsortEquiheightCostModel]
syntethical_workload_lost = [Random,SeqOver,SeqRand,ZoomIn,SeqZoomIn,Skew,ZoomOutAlt,Periodic,ZoomInAlt]
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

def generate_cost_model(COLUMN_SIZE):
    print("Generating Cost Model Constants")
    os.system('g++ -std=c++11 -O3 scripts/cost_model/measure.cpp -march=native')
    os.system('rm src/include/progressive/constants.h')
    result = os.popen("./a.out "+str(COLUMN_SIZE)).read()
    os.system('rm ./a.out')
    file = open('src/include/progressive/constants.h',"w")
    file.write('#ifndef PROGRESSIVEINDEXING_CONSTANTS_H\n')
    file.write('#define PROGRESSIVEINDEXING_CONSTANTS_H\n\n')
    file.write('#define PAGESIZE 4096.0\n')
    file.write('#define ELEMENTS_PER_PAGE (PAGESIZE / sizeof(int64_t))\n\n')
    file.write(result + '\n')
    file.write('\n#endif //PROGRESSIVEINDEXING_CONSTANTS_H')
    file.close() 

def run_experiment_print(COLUMN_SIZE,QUERY_PATTERN,QUERY_SELECTIVITY,ALGORITHM,NUM_QUERIES,FIXED_DELTA=None,INTERACTIVITY_THRESHOLD=None):
    COLUMN_PATH = column_path(COLUMN_SIZE)
    QUERY_PATH = query_path(COLUMN_PATH,QUERY_SELECTIVITY,QUERY_PATTERN)
    ANSWER_PATH = answer_path(COLUMN_PATH,QUERY_SELECTIVITY,QUERY_PATTERN)
    codestr ="./main --num-queries=" + str(NUM_QUERIES) + " --column-size=" + str(COLUMN_SIZE) + \
             " --algorithm="+str(ALGORITHM)+ " --column-path=" + str(COLUMN_PATH + "column") + " --query-path=" \
             + str(QUERY_PATH) + " --answer-path=" + str(ANSWER_PATH) + " --delta=" + str(FIXED_DELTA) + " --interactivity-threshold=" + str(INTERACTIVITY_THRESHOLD) + " --correctness=" + str(0)
    print(codestr)
    result = os.popen(codestr).read()
    print(result)

def run_experiment_baseline(COLUMN_SIZE,QUERY_PATTERN,QUERY_SELECTIVITY,ALGORITHM,NUM_QUERIES):
    COLUMN_PATH = column_path(COLUMN_SIZE)
    QUERY_PATH = query_path(COLUMN_PATH,QUERY_SELECTIVITY,QUERY_PATTERN)
    ANSWER_PATH = answer_path(COLUMN_PATH,QUERY_SELECTIVITY,QUERY_PATTERN)
    codestr ="./main --num-queries=" + str(NUM_QUERIES) + " --column-size=" + str(COLUMN_SIZE) + \
             " --algorithm="+str(ALGORITHM)+ " --column-path=" + str(COLUMN_PATH + "column") + " --query-path=" \
             + str(QUERY_PATH) + " --answer-path=" + str(ANSWER_PATH) + " --correctness=" + str(0)
    print(codestr)
    result = os.popen(codestr).read()
    cursor.execute('''INSERT INTO experiment(algorithm_id, workload_id, column_size, query_selectivity)
                  VALUES(:algorithm_id,:workload_id, :column_size, :query_selectivity)''',
                  {'algorithm_id':ALGORITHM, 'workload_id':QUERY_PATTERN, 'column_size':COLUMN_SIZE, 'query_selectivity':QUERY_SELECTIVITY})
    experiment_id =  cursor.execute('''SELECT id FROM experiment where algorithm_id = (?) and workload_id=(?) and column_size=(?) and query_selectivity=(?)
                    ''', (ALGORITHM,QUERY_PATTERN,COLUMN_SIZE,QUERY_SELECTIVITY))
    experiment_id = cursor.fetchone()
    experiment_id = experiment_id[0]
    result = result.split("\n")
    for query_number in range(0, len(result)-1):
        query_result = result[query_number].split(";")
        cursor.execute('''INSERT INTO queries(experiment_id, query_number, query_time,indexing_time,total_time)
              VALUES(:experiment_id,:query_number, :query_time, :indexing_time, :total_time)''',
              {'experiment_id':experiment_id, 'query_number':query_number, 'query_time':query_result[1], 'indexing_time':query_result[2], 'total_time':query_result[3]})

def run_experiment_progressive(COLUMN_SIZE,QUERY_PATTERN,QUERY_SELECTIVITY,ALGORITHM,NUM_QUERIES,FIXED_DELTA):
    COLUMN_PATH = column_path(COLUMN_SIZE)
    QUERY_PATH = query_path(COLUMN_PATH,QUERY_SELECTIVITY,QUERY_PATTERN)
    ANSWER_PATH = answer_path(COLUMN_PATH,QUERY_SELECTIVITY,QUERY_PATTERN)
    codestr ="./main --num-queries=" + str(NUM_QUERIES) + " --column-size=" + str(COLUMN_SIZE) + \
             " --algorithm="+str(ALGORITHM)+ " --column-path=" + str(COLUMN_PATH + "column") + " --query-path=" \
             + str(QUERY_PATH) + " --answer-path=" + str(ANSWER_PATH) + " --delta=" + str(FIXED_DELTA) + " --correctness=" + str(0)
    print(codestr)
    result = os.popen(codestr).read()
    cursor.execute('''INSERT INTO experiment(algorithm_id, workload_id, column_size, query_selectivity,fixed_delta)
                  VALUES(:algorithm_id,:workload_id, :column_size, :query_selectivity,:fixed_delta)''',
                  {'algorithm_id':ALGORITHM, 'workload_id':QUERY_PATTERN, 'column_size':COLUMN_SIZE, 'query_selectivity':QUERY_SELECTIVITY, 'fixed_delta':FIXED_DELTA})
    experiment_id =  cursor.execute('''SELECT id FROM experiment where algorithm_id = (?) and workload_id=(?) and column_size=(?) and query_selectivity=(?) and fixed_delta=(?)
                    ''', (ALGORITHM,QUERY_PATTERN,COLUMN_SIZE,QUERY_SELECTIVITY,FIXED_DELTA))
    experiment_id = cursor.fetchone()
    experiment_id = experiment_id[0]
    result = result.split("\n")
    for query_number in range(0, len(result)-1):
        query_result = result[query_number].split(";")
        cursor.execute('''INSERT INTO queries(experiment_id, query_number, query_time,indexing_time,total_time)
              VALUES(:experiment_id,:query_number, :query_time, :indexing_time, :total_time)''',
              {'experiment_id':experiment_id, 'query_number':query_number, 'query_time':query_result[1], 'indexing_time':query_result[2], 'total_time':query_result[3]})
# query_time REAL NOT NULL,indexing_time REAL NOT NULL,total_time 
def run_experiment_cost_model(COLUMN_SIZE,QUERY_PATTERN,QUERY_SELECTIVITY,ALGORITHM,NUM_QUERIES,FIXED_INTERACTIVITY_THRESHOLD):
    COLUMN_PATH = column_path(COLUMN_SIZE)
    QUERY_PATH = query_path(COLUMN_PATH,QUERY_SELECTIVITY,QUERY_PATTERN)
    ANSWER_PATH = answer_path(COLUMN_PATH,QUERY_SELECTIVITY,QUERY_PATTERN)
    codestr ="./main --num-queries=" + str(NUM_QUERIES) + " --column-size=" + str(COLUMN_SIZE) + \
             " --algorithm="+str(ALGORITHM)+ " --column-path=" + str(COLUMN_PATH + "column") + " --query-path=" \
             + str(QUERY_PATH) + " --answer-path=" + str(ANSWER_PATH)  + " --interactivity-threshold=" + str(FIXED_INTERACTIVITY_THRESHOLD) + " --correctness=" + str(0)
    print(codestr)
    result = os.popen(codestr).read()
    cursor.execute('''INSERT INTO experiment(algorithm_id, workload_id, column_size, query_selectivity,fixed_interactivity_threshold)
                  VALUES(:algorithm_id,:workload_id, :column_size, :query_selectivity,:fixed_interactivity_threshold)''',
                  {'algorithm_id':ALGORITHM, 'workload_id':QUERY_PATTERN, 'column_size':COLUMN_SIZE, 'query_selectivity':QUERY_SELECTIVITY, 'fixed_interactivity_threshold':FIXED_INTERACTIVITY_THRESHOLD})
    experiment_id =  cursor.execute('''SELECT id FROM experiment where algorithm_id = (?) and workload_id=(?) and column_size=(?) and query_selectivity=(?) and fixed_interactivity_threshold=(?)
                    ''', (ALGORITHM,QUERY_PATTERN,COLUMN_SIZE,QUERY_SELECTIVITY,FIXED_INTERACTIVITY_THRESHOLD))
    experiment_id = cursor.fetchone()
    experiment_id = experiment_id[0]
    result = result.split("\n")
    for query_number in range(0, len(result)-1):
        query_result = result[query_number].split(";")
        cursor.execute('''INSERT INTO queries(experiment_id, query_number, delta, query_time,indexing_time,total_time)
              VALUES(:experiment_id,:query_number, :delta, :query_time, :indexing_time, :total_time)''',
              {'experiment_id':experiment_id, 'query_number':query_number, 'delta':query_result[0], 'query_time':query_result[1], 'indexing_time':query_result[2], 'total_time':query_result[3]})


def template_run(ALGORITHM_LIST,DELTA_LIST=0,COLUMN_SIZE_LIST=0,WORKLOAD_LIST=0,QUERY_SELECTIVITY_LIST=0,INTERACTIVITY_THRESHOLD_LIST=0,NUM_QUERIES=10000):
    compile()
    if COLUMN_SIZE_LIST == 0:
        COLUMN_SIZE_LIST = [10000000,100000000,1000000000]
    if WORKLOAD_LIST == 0:
        WORKLOAD_LIST = syntethical_workload_lost
    if INTERACTIVITY_THRESHOLD_LIST == 0:
        INTERACTIVITY_THRESHOLD_LIST = [0.8, 1.2, 1.5,2]
    if DELTA_LIST == 0:
        DELTA_LIST = [0.005,0.01,0.05,0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0]
    for column_size in COLUMN_SIZE_LIST:
        generate_cost_model(column_size) #Radix MSD Cost Model is dependent on column_size
        generate_column(column_size)
        if QUERY_SELECTIVITY_LIST == 0:
            if column_size == 10000000:
                QUERY_SELECTIVITY_LIST = [0.00001,0.01,10,50]
            if column_size == 100000000:
                QUERY_SELECTIVITY_LIST = [0.000001,0.01,10,50]
            if column_size == 1000000000:
                QUERY_SELECTIVITY_LIST = [0.000001,0.01,10,50]
            else:
                QUERY_SELECTIVITY_LIST = [0.001]
        for query in WORKLOAD_LIST:
            for selectivity in QUERY_SELECTIVITY_LIST:
                generate_workload(NUM_QUERIES,column_size,query,selectivity)
                for algorithm in ALGORITHM_LIST:
                    if algorithm in baseline_list:
                        cursor.execute('''
                           SELECT id FROM experiment where algorithm_id = (?) and workload_id=(?) and column_size=(?) and query_selectivity=(?)
                        ''', (algorithm,query,column_size,selectivity))
                        experiment_exists = cursor.fetchone()
                        if experiment_exists is None:
                            run_experiment_baseline(column_size,query,selectivity,algorithm,NUM_QUERIES)
                        db.commit()
                    if algorithm in progressive_list:
                        for delta in DELTA_LIST:
                            cursor.execute('''
                               SELECT id FROM experiment where algorithm_id = (?) and workload_id=(?) and column_size=(?) and query_selectivity=(?) and fixed_delta=(?)
                            ''', (algorithm,query,column_size,selectivity,delta))
                            experiment_exists = cursor.fetchone()
                            if experiment_exists is None:
                                run_experiment_progressive(column_size,query,selectivity,algorithm,NUM_QUERIES,delta)
                            db.commit()
                    if algorithm in progressive_cm_list:
                        for interactivity_threshold in INTERACTIVITY_THRESHOLD_LIST:
                            cursor.execute('''
                               SELECT id FROM experiment where algorithm_id = (?) and workload_id=(?) and column_size=(?) and query_selectivity=(?) and fixed_interactivity_threshold=(?)
                            ''', (algorithm,query,column_size,selectivity,interactivity_threshold))
                            experiment_exists = cursor.fetchone()
                            if experiment_exists is None:
                                run_experiment_cost_model(column_size,query,selectivity,algorithm,NUM_QUERIES,interactivity_threshold)
                            db.commit()
def run_baseline():
    ALGORITHM_LIST = baseline_list
    # COLUMN_SIZE_LIST=[1000000]
    # WORKLOAD_LIST=[]
    # DELTA_LIST=[]
    # QUERY_SELECTIVITY_LIST=[]
    # INTERACTIVITY_THRESHOLD_LIST=[]
    # NUM_QUERIES=10
    template_run(ALGORITHM_LIST)

def run_progressive():
    ALGORITHM_LIST = progressive_list
    # COLUMN_SIZE_LIST=[1000000]
    # WORKLOAD_LIST=[]
    # DELTA_LIST=[]
    # QUERY_SELECTIVITY_LIST=[]
    # INTERACTIVITY_THRESHOLD_LIST=[]
    # NUM_QUERIES=10
    template_run(ALGORITHM_LIST)

def run_progressive_cost_model():
    ALGORITHM_LIST = progressive_cm_list
    # COLUMN_SIZE_LIST=[1000000]
    # WORKLOAD_LIST=[]
    # DELTA_LIST=[]
    # QUERY_SELECTIVITY_LIST=[]
    # INTERACTIVITY_THRESHOLD_LIST=[]
    # NUM_QUERIES=10
    template_run(ALGORITHM_LIST)

def run():
    ALGORITHM_LIST=[]
    COLUMN_SIZE_LIST=[]
    WORKLOAD_LIST=[]
    DELTA_LIST=[]
    QUERY_SELECTIVITY_LIST=[]
    INTERACTIVITY_THRESHOLD_LIST=[]
    NUM_QUERIES=[]

run_baseline()
# run_progressive()
# run_progressive_cost_model()
db.close()