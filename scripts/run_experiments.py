import os
import inspect
import sqlite3
import urllib

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

#Setting Values for Different Column Distributions
RandomDist = 1
SkewedDist = 2
SkyServerDist = 3


baseline_list = [FullScan,FullIndex,StandardCracking,StochasticCracking,ProgressiveStochasticCracking,CoarseGranularIndex]
progressive_list = [ProgressiveQuicksort,ProgressiveRadixsortMSD, ProgressiveRadixsortLSD, ProgressiveBucketsortEquiheight]
progressive_cm_list = [ProgressiveQuicksortCostModel,ProgressiveRadixsortMSDCostModel, ProgressiveRadixsortLSDCostModel, ProgressiveBucketsortEquiheightCostModel]
syntethical_workload_list = [Random,SeqOver,SeqRand,ZoomIn,SeqZoomIn,Skew,ZoomOutAlt,Periodic,ZoomInAlt]

def DownloadSkyServer():
    if (os.path.isdir("./real_data") == False):
        print("Downloading SkyServer Data")
        os.system("mkdir -p real_data/skyserver")
        urllib.urlretrieve ("https://zenodo.org/record/2557531/files/answer?download=1", "real_data/skyserver/answer")
        urllib.urlretrieve ("https://zenodo.org/record/2557531/files/answer_0.001?download=1", "real_data/skyserver/answer_0.001")
        urllib.urlretrieve ("https://zenodo.org/record/2557531/files/answer_0.1?download=1", "real_data/skyserver/answer_0.1")
        urllib.urlretrieve ("https://zenodo.org/record/2557531/files/answer_1?download=1", "real_data/skyserver/answer_1")
        urllib.urlretrieve ("https://zenodo.org/record/2557531/files/answer_1e-05?download=1", "real_data/skyserver/answer_1e-05")
        urllib.urlretrieve ("https://zenodo.org/record/2557531/files/answer_2e-07?download=1", "real_data/skyserver/answer_2e-07")
        urllib.urlretrieve ("https://zenodo.org/record/2557531/files/query?download=1", "real_data/skyserver/query")
        urllib.urlretrieve ("https://zenodo.org/record/2557531/files/query_0.001?download=1", "real_data/skyserver/query_0.001")
        urllib.urlretrieve ("https://zenodo.org/record/2557531/files/query_0.1?download=1", "real_data/skyserver/query_0.1")
        urllib.urlretrieve ("https://zenodo.org/record/2557531/files/query_1?download=1", "real_data/skyserver/query_1")
        urllib.urlretrieve ("https://zenodo.org/record/2557531/files/query_1e-05?download=1", "real_data/skyserver/query_1e-05")
        urllib.urlretrieve ("https://zenodo.org/record/2557531/files/query_2e-07?download=1", "real_data/skyserver/query_2e-07")
        urllib.urlretrieve ("https://zenodo.org/record/2557531/files/skyserver.data?download=1", "real_data/skyserver/skyserver.data")
        urllib.urlretrieve ("https://zenodo.org/record/2557531/files/skyserver.queries?download=1", "real_data/skyserver/skyserver.queries")

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
    COLUMN_PATH = COLUMN_PATH + 'column'
    if QUERY_PATTERN == SkyServer:
        COLUMN_PATH = "real_data/skyserver/skyserver.data"
        QUERY_PATH = "real_data/skyserver/query_"+str(QUERY_SELECTIVITY)
        ANSWER_PATH = "real_data/skyserver/answer_"+str(QUERY_SELECTIVITY)
    codestr ="./main --num-queries=" + str(NUM_QUERIES) + " --column-size=" + str(COLUMN_SIZE) + \
             " --algorithm="+str(ALGORITHM)+ " --column-path=" + str(COLUMN_PATH) + " --query-path=" \
             + str(QUERY_PATH) + " --answer-path=" + str(ANSWER_PATH) + " --correctness=" + str(0)
    print(codestr)
    result = os.popen(codestr).read()
    cursor.execute('''INSERT INTO experiments(algorithm_id, workload_id, column_size, query_selectivity)
                  VALUES(:algorithm_id,:workload_id, :column_size, :query_selectivity)''',
                  {'algorithm_id':ALGORITHM, 'workload_id':QUERY_PATTERN, 'column_size':COLUMN_SIZE, 'query_selectivity':QUERY_SELECTIVITY})
    experiment_id =  cursor.execute('''SELECT id FROM experiments where algorithm_id = (?) and workload_id=(?) and column_size=(?) and query_selectivity=(?)
                    ''', (ALGORITHM,QUERY_PATTERN,COLUMN_SIZE,QUERY_SELECTIVITY))
    experiment_id = cursor.fetchone()
    experiment_id = experiment_id[0]
    result = result.split("\n")
    for query_number in range(0, len(result)-1):
        query_result = result[query_number].split(";")
        cursor.execute('''INSERT INTO queries(experiment_id, query_number, query_time,indexing_time,total_time,pref_sum_total_time)
              VALUES(:experiment_id,:query_number, :query_time, :indexing_time, :total_time,:pref_sum_total_time)''',
              {'experiment_id':experiment_id, 'query_number':query_number, 'query_time':query_result[1], 'indexing_time':query_result[2], 'total_time':query_result[3], 'pref_sum_total_time': query_result[4]})

def run_experiment_progressive(COLUMN_SIZE,QUERY_PATTERN,QUERY_SELECTIVITY,ALGORITHM,NUM_QUERIES,FIXED_DELTA):
    COLUMN_PATH = column_path(COLUMN_SIZE)
    QUERY_PATH = query_path(COLUMN_PATH,QUERY_SELECTIVITY,QUERY_PATTERN)
    ANSWER_PATH = answer_path(COLUMN_PATH,QUERY_SELECTIVITY,QUERY_PATTERN)
    COLUMN_PATH = COLUMN_PATH + 'column'
    if QUERY_PATTERN == SkyServer:
        COLUMN_PATH = "real_data/skyserver/skyserver.data"
        QUERY_PATH = "real_data/skyserver/query_"+str(QUERY_SELECTIVITY)
        ANSWER_PATH = "real_data/skyserver/answer_"+str(QUERY_SELECTIVITY)
    codestr ="./main --num-queries=" + str(NUM_QUERIES) + " --column-size=" + str(COLUMN_SIZE) + \
             " --algorithm="+str(ALGORITHM)+ " --column-path=" + str(COLUMN_PATH) + " --query-path=" \
             + str(QUERY_PATH) + " --answer-path=" + str(ANSWER_PATH) + " --delta=" + str(FIXED_DELTA) + " --correctness=" + str(0)
    print(codestr)
    result = os.popen(codestr).read()
    cursor.execute('''INSERT INTO experiments(algorithm_id, workload_id, column_size, query_selectivity,fixed_delta)
                  VALUES(:algorithm_id,:workload_id, :column_size, :query_selectivity,:fixed_delta)''',
                  {'algorithm_id':ALGORITHM, 'workload_id':QUERY_PATTERN, 'column_size':COLUMN_SIZE, 'query_selectivity':QUERY_SELECTIVITY, 'fixed_delta':FIXED_DELTA})
    experiment_id =  cursor.execute('''SELECT id FROM experiments where algorithm_id = (?) and workload_id=(?) and column_size=(?) and query_selectivity=(?) and fixed_delta=(?)
                    ''', (ALGORITHM,QUERY_PATTERN,COLUMN_SIZE,QUERY_SELECTIVITY,FIXED_DELTA))
    experiment_id = cursor.fetchone()
    experiment_id = experiment_id[0]
    result = result.split("\n")
    for query_number in range(0, len(result)-1):
        query_result = result[query_number].split(";")
        cursor.execute('''INSERT INTO queries(experiment_id, query_number, query_time,indexing_time,total_time,pref_sum_total_time)
              VALUES(:experiment_id,:query_number, :query_time, :indexing_time, :total_time,:pref_sum_total_time)''',
              {'experiment_id':experiment_id, 'query_number':query_number, 'query_time':query_result[1], 'indexing_time':query_result[2], 'total_time':query_result[3],'pref_sum_total_time':query_result[4]})

def run_experiment_cost_model(COLUMN_SIZE,QUERY_PATTERN,QUERY_SELECTIVITY,ALGORITHM,NUM_QUERIES,FIXED_INTERACTIVITY_THRESHOLD,COLUMN_DISTRIBUTION,INTERACTIVITY_IS_PERCENTAGE=1,QUERY_DECAY = 0):
    COLUMN_PATH = column_path(COLUMN_SIZE)
    QUERY_PATH = query_path(COLUMN_PATH,QUERY_SELECTIVITY,QUERY_PATTERN)
    ANSWER_PATH = answer_path(COLUMN_PATH,QUERY_SELECTIVITY,QUERY_PATTERN)
    COLUMN_PATH = COLUMN_PATH + 'column'
    if QUERY_PATTERN == SkyServer:
        COLUMN_PATH = "real_data/skyserver/skyserver.data"
        QUERY_PATH = "real_data/skyserver/query_"+str(QUERY_SELECTIVITY)
        ANSWER_PATH = "real_data/skyserver/answer_"+str(QUERY_SELECTIVITY)
    codestr ="./main --num-queries=" + str(NUM_QUERIES) + " --column-size=" + str(COLUMN_SIZE) + \
             " --algorithm="+str(ALGORITHM)+ " --column-path=" + str(COLUMN_PATH) + " --query-path=" \
             + str(QUERY_PATH) + " --answer-path=" + str(ANSWER_PATH)  + " --interactivity-threshold=" + str(FIXED_INTERACTIVITY_THRESHOLD) \
             + " --correctness=" + str(0) + " --interactivity-is-percentage="+str(INTERACTIVITY_IS_PERCENTAGE) + " --decay-queries="+str(QUERY_DECAY)
    print(codestr)
    result = os.popen(codestr).read()
    cursor.execute('''INSERT INTO experiments(algorithm_id, workload_id, column_size, query_selectivity,fixed_interactivity_threshold)
                  VALUES(:algorithm_id,:workload_id, :column_size, :query_selectivity,:fixed_interactivity_threshold)''',
                  {'algorithm_id':ALGORITHM, 'workload_id':QUERY_PATTERN, 'column_size':COLUMN_SIZE, 'query_selectivity':QUERY_SELECTIVITY, 'fixed_interactivity_threshold':FIXED_INTERACTIVITY_THRESHOLD})
    experiment_id =  cursor.execute('''SELECT id FROM experiments where algorithm_id = (?) and workload_id=(?) and column_size=(?) and query_selectivity=(?) and fixed_interactivity_threshold=(?)
                    ''', (ALGORITHM,QUERY_PATTERN,COLUMN_SIZE,QUERY_SELECTIVITY,FIXED_INTERACTIVITY_THRESHOLD))
    experiment_id = cursor.fetchone()
    experiment_id = experiment_id[0]
    result = result.split("\n")
    for query_number in range(0, len(result)-1):
        query_result = result[query_number].split(";")
        cursor.execute('''INSERT INTO queries(experiment_id, query_number, delta, query_time,indexing_time,total_time,pref_sum_total_time)
              VALUES(:experiment_id,:query_number, :delta, :query_time, :indexing_time, :total_time, :pref_sum_total_time)''',
              {'experiment_id':experiment_id, 'query_number':query_number, 'delta':query_result[0], 'query_time':query_result[1], 'indexing_time':query_result[2], 'total_time':query_result[3], 'pref_sum_total_time': query_result[4]})


def template_run(ALGORITHM_LIST,DELTA_LIST=0,COLUMN_SIZE_LIST=0,WORKLOAD_LIST=0,QUERY_SELECTIVITY_LIST=0,INTERACTIVITY_THRESHOLD_LIST=0,NUM_QUERIES=10000,INTERACTIVITY_IS_PERCENTAGE=1):
    generate_cost_model(100000000) #Mock Gen
    compile()
    if COLUMN_SIZE_LIST == 0:
        COLUMN_SIZE_LIST = [10000000,100000000,1000000000]
    if WORKLOAD_LIST == 0:
        WORKLOAD_LIST = syntethical_workload_list
    if INTERACTIVITY_THRESHOLD_LIST == 0:
        INTERACTIVITY_THRESHOLD_LIST = [0.8, 1.2, 1.5,2]
    if DELTA_LIST == 0:
        DELTA_LIST = [0.005,0.01,0.05,0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0]
    for column_size in COLUMN_SIZE_LIST:
        generate_cost_model(column_size) #Radix MSD Cost Model is dependent on column_size
        generate_column(column_size)
        if column_size == 10000000:
            QUERY_SELECTIVITY_LIST = [0.00001,0.01,1,10]
        elif column_size == 100000000:
            QUERY_SELECTIVITY_LIST = [0.000001,0.01,1,10]
        elif column_size == 1000000000:
            QUERY_SELECTIVITY_LIST = [0.0000001,0.01,1,10]
        else:
            QUERY_SELECTIVITY_LIST = [0.001]
        for query in WORKLOAD_LIST:
            for selectivity in QUERY_SELECTIVITY_LIST:
                generate_workload(NUM_QUERIES,column_size,query,selectivity)
                for algorithm in ALGORITHM_LIST:
                    if algorithm in baseline_list:
                        cursor.execute('''
                           SELECT id FROM experiments where algorithm_id = (?) and workload_id=(?) and column_size=(?) and query_selectivity=(?)
                        ''', (algorithm,query,column_size,selectivity))
                        experiment_exists = cursor.fetchone()
                        if experiment_exists is None:
                            run_experiment_baseline(column_size,query,selectivity,algorithm,NUM_QUERIES)
                        db.commit()
                    if algorithm in progressive_list:
                        for delta in DELTA_LIST:
                            cursor.execute('''
                               SELECT id FROM experiments where algorithm_id = (?) and workload_id=(?) and column_size=(?) and query_selectivity=(?) and fixed_delta=(?)
                            ''', (algorithm,query,column_size,selectivity,delta))
                            experiment_exists = cursor.fetchone()
                            if experiment_exists is None:
                                run_experiment_progressive(column_size,query,selectivity,algorithm,NUM_QUERIES,delta)
                            db.commit()
                    if algorithm in progressive_cm_list:
                        if INTERACTIVITY_IS_PERCENTAGE == 0:
                            # Query DB
                            cursor.execute('''
                               SELECT min(total_time) from queries inner join experiments on (experiments.id = queries.experiment_id) where algorithm_id in (2,3,4,5,6) and column_size = (?) and workload_id = (?) and query_selectivity = (?) and query_number == 0
                            ''', (column_size,query,selectivity))
                            interactivity_threshold = cursor.fetchone()
                            cursor.execute('''
                               SELECT id FROM experiments where algorithm_id = (?) and workload_id=(?) and column_size=(?) and query_selectivity=(?) and fixed_interactivity_threshold=(?)
                            ''', (algorithm,query,column_size,selectivity,interactivity_threshold[0]))
                            experiment_exists = cursor.fetchone()
                            if experiment_exists is None:
                                run_experiment_cost_model(column_size,query,selectivity,algorithm,NUM_QUERIES,interactivity_threshold[0],INTERACTIVITY_IS_PERCENTAGE)
                            db.commit()
                        else:
                            for interactivity_threshold in INTERACTIVITY_THRESHOLD_LIST:
                                cursor.execute('''
                                   SELECT id FROM experiments where algorithm_id = (?) and workload_id=(?) and column_size=(?) and query_selectivity=(?) and fixed_interactivity_threshold=(?)
                                ''', (algorithm,query,column_size,selectivity,interactivity_threshold))
                                experiment_exists = cursor.fetchone()
                                if experiment_exists is None:
                                    run_experiment_cost_model(column_size,query,selectivity,algorithm,NUM_QUERIES,interactivity_threshold,INTERACTIVITY_IS_PERCENTAGE)
                                db.commit()

def run_skyserver(ALGORITHM_LIST,DELTA_LIST=0,INTERACTIVITY_THRESHOLD_LIST=0,QUERY_SELECTIVITY_LIST=0,NUM_QUERIES = 158325, INTERACTIVITY_IS_PERCENTAGE=1, QUERY_DECAY=0):
    DownloadSkyServer()
    generate_cost_model(100000000) #Mock Gen
    compile()
    COLUMN_PATH = "real_data/skyserver/skyserver.data"
    QUERY_PATH = "real_data/skyserver/query"
    ANSWER_PATH = "real_data/skyserver/answer"
    column_size = 585624220
    CORRECTNESS = 0
    query = SkyServer
    if QUERY_SELECTIVITY_LIST == 0:
        QUERY_SELECTIVITY_LIST = [0.1]
    for query_sel in QUERY_SELECTIVITY_LIST:
        codestr = "./generate_workload --num-queries=" + str(NUM_QUERIES) + " --column-size=" + str(column_size)  \
                  + " --column-path=" + str(COLUMN_PATH) + " --query-path=" + str(QUERY_PATH)+"_"+str(query_sel) + " --answer-path=" + str(ANSWER_PATH)+"_"+str(query_sel) + " --selectivity=" \
                  + str(query_sel) + " --queries-pattern=" +  str(query)
        print (codestr)
        os.system(codestr)
    if INTERACTIVITY_THRESHOLD_LIST == 0:
        INTERACTIVITY_THRESHOLD_LIST = [0.8, 1.2, 1.5,2]
    if DELTA_LIST == 0:
        DELTA_LIST = [0.005,0.01,0.05,0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0]
    generate_cost_model(column_size) #Radix MSD Cost Model is dependent on column_size
    for selectivity in QUERY_SELECTIVITY_LIST:
        for algorithm in ALGORITHM_LIST:
            if algorithm in baseline_list:
                cursor.execute('''
                   SELECT id FROM experiments where algorithm_id = (?) and workload_id=(?) and column_size=(?) and query_selectivity=(?) and column_distribution_id=(?)
                ''', (algorithm,query,column_size,selectivity,SkyServerDist))
                experiment_exists = cursor.fetchone()
                if experiment_exists is None:
                    run_experiment_baseline(column_size,query,selectivity,algorithm,NUM_QUERIES,SkyServerDist)
                db.commit()
            if algorithm in progressive_list:
                for delta in DELTA_LIST:
                    cursor.execute('''
                       SELECT id FROM experiments where algorithm_id = (?) and workload_id=(?) and column_size=(?) and query_selectivity=(?) and fixed_delta=(?)
                    ''', (algorithm,query,column_size,selectivity,delta))
                    experiment_exists = cursor.fetchone()
                    if experiment_exists is None:
                        run_experiment_progressive(column_size,query,selectivity,algorithm,NUM_QUERIES,SkyServerDist,delta)
                    db.commit()
            if algorithm in progressive_cm_list:
                if QUERY_DECAY != 0:
                    cursor.execute('''
                           SELECT id FROM experiments where algorithm_id = (?) and workload_id=(?) and column_size=(?) and query_selectivity=(?) and fixed_interactivity_threshold=(?) and column_distribution_id=(?)
                        ''', (algorithm,query,column_size,selectivity,1.2,SkyServerDist))
                    experiment_exists = cursor.fetchone()
                    if experiment_exists is None:
                        run_experiment_cost_model(column_size,query,selectivity,algorithm,NUM_QUERIES,1.2,INTERACTIVITY_IS_PERCENTAGE,SkyServerDist,QUERY_DECAY)
                    db.commit()
                if INTERACTIVITY_IS_PERCENTAGE == 0:
                    # Query DB
                    cursor.execute('''
                       SELECT min(total_time) from queries inner join experiments on (experiments.id = queries.experiment_id) where algorithm_id in (2,3,4,5,6) and column_size = (?) and workload_id = (?) and query_selectivity = (?) and query_number == 0
                    ''', (column_size,query,selectivity))
                    interactivity_threshold = cursor.fetchone()
                    cursor.execute('''
                       SELECT id FROM experiments where algorithm_id = (?) and workload_id=(?) and column_size=(?) and query_selectivity=(?) and fixed_interactivity_threshold=(?) and column_distribution_id=(?)
                    ''', (algorithm,query,column_size,selectivity,interactivity_threshold[0],SkyServerDist))
                    experiment_exists = cursor.fetchone()
                    if experiment_exists is None:
                        run_experiment_cost_model(column_size,query,selectivity,algorithm,NUM_QUERIES,SkyServerDist,interactivity_threshold[0],INTERACTIVITY_IS_PERCENTAGE)
                    db.commit()
                else:
                    for interactivity_threshold in INTERACTIVITY_THRESHOLD_LIST:
                        cursor.execute('''
                           SELECT id FROM experiments where algorithm_id = (?) and workload_id=(?) and column_size=(?) and query_selectivity=(?) and fixed_interactivity_threshold=(?) and column_distribution_id=(?)
                        ''', (algorithm,query,column_size,selectivity,interactivity_threshold,SkyServerDist))
                        experiment_exists = cursor.fetchone()
                        if experiment_exists is None:
                            run_experiment_cost_model(column_size,query,selectivity,algorithm,NUM_QUERIES,interactivity_threshold,SkyServerDist,INTERACTIVITY_IS_PERCENTAGE)
                        db.commit()
def run_baseline():
    ALGORITHM_LIST = baseline_list
    # COLUMN_SIZE_LIST=[]
    # WORKLOAD_LIST=[]
    # DELTA_LIST=[]
    # QUERY_SELECTIVITY_LIST=[]
    # INTERACTIVITY_THRESHOLD_LIST=[]
    # NUM_QUERIES=10
    template_run(ALGORITHM_LIST)

def run_progressive():
    ALGORITHM_LIST = progressive_list
    # COLUMN_SIZE_LIST=[1000]
    # WORKLOAD_LIST=[Random]
    # DELTA_LIST=[0.25]
    # QUERY_SELECTIVITY_LIST=[1]
    # INTERACTIVITY_THRESHOLD_LIST=[]
    # NUM_QUERIES=100
    template_run(ALGORITHM_LIST)

def run_progressive_cost_model():
    ALGORITHM_LIST = progressive_cm_list
    # COLUMN_SIZE_LIST=[1000]
    # WORKLOAD_LIST=[]
    # DELTA_LIST=[]
    # QUERY_SELECTIVITY_LIST=[]
    # INTERACTIVITY_THRESHOLD_LIST=[]
    # NUM_QUERIES=10
    template_run(ALGORITHM_LIST)

def run_skyserver_baseline():
    ALGORITHM_LIST = baseline_list
    # QUERY_SELECTIVITY_LIST=[0.001]
    run_skyserver(ALGORITHM_LIST)

def run_skyserver_progressive():
    ALGORITHM_LIST = progressive_list
    # DELTA_LIST=[]
    # QUERY_SELECTIVITY_LIST=[0.001]
    run_skyserver(ALGORITHM_LIST)

def run_skyserver_progressive_cost_model():
    ALGORITHM_LIST = progressive_cm_list
    # QUERY_SELECTIVITY_LIST=[0.001]
    # INTERACTIVITY_THRESHOLD_LIST=[1.2]
    run_skyserver(ALGORITHM_LIST)

# Threshold = cheapest first query of baseline
# This experiment depends on run_skyserver_baseline
def run_skyserver_progressive_cost_model_cracking_threshold():
    ALGORITHM_LIST = progressive_cm_list
    # DELTA_LIST=[]
    # QUERY_SELECTIVITY_LIST=[]
    # INTERACTIVITY_THRESHOLD_LIST=[]
    run_skyserver(ALGORITHM_LIST,INTERACTIVITY_IS_PERCENTAGE=0)

def run_progressive_cost_model_cracking_threshold():
    ALGORITHM_LIST = progressive_cm_list
    # DELTA_LIST=[]
    # QUERY_SELECTIVITY_LIST=[]
    # INTERACTIVITY_THRESHOLD_LIST=[]
    template_run(ALGORITHM_LIST,INTERACTIVITY_IS_PERCENTAGE=0)

def run_skyserver_progressive_cost_model_query_decay():
    ALGORITHM_LIST = [ProgressiveQuicksortCostModel]
    NUM_QUERY_DECAY=300
    QUERY_SELECTIVITY_LIST=[0.001]
    INTERACTIVITY_THRESHOLD_LIST=[1.2]
    run_skyserver(ALGORITHM_LIST,QUERY_DECAY=NUM_QUERY_DECAY,QUERY_SELECTIVITY_LIST=QUERY_SELECTIVITY_LIST,INTERACTIVITY_THRESHOLD_LIST=INTERACTIVITY_THRESHOLD_LIST)

# Only running first query of full scan
def run_fullscan_all():
    ALGORITHM_LIST = [FullScan]
    template_run(ALGORITHM_LIST)
    run_skyserver(ALGORITHM_LIST)

def run():
    ALGORITHM_LIST=[]
    COLUMN_SIZE_LIST=[]
    WORKLOAD_LIST=[]
    DELTA_LIST=[]
    QUERY_SELECTIVITY_LIST=[]
    INTERACTIVITY_THRESHOLD_LIST=[]
    NUM_QUERIES=[]

run_baseline()
run_progressive()
run_progressive_cost_model()
run_skyserver_baseline()
run_skyserver_progressive()
run_skyserver_progressive_cost_model()
# run_skyserver_progressive_cost_model_cracking_threshold()
# run_progressive_cost_model_cracking_threshold()
# run_skyserver_progressive_cost_model_query_decay()
# run_skyserver_progressive_cost_model_query_decay()
db.close()