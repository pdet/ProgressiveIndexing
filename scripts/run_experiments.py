import os
import inspect
import sqlite3
import urllib

# os.system("rm results.db")
if (os.path.exists("./results.db") == False):
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
ProgressiveQuicksort=1
ProgressiveQuicksortCostModel=2
ProgressiveBucketsortEquiheight=3
ProgressiveBucketsortEquiheightCostModel=4
ProgressiveRadixsortLSD=5
ProgressiveRadixsortLSDCostModel=6
ProgressiveRadixsortMSDPure=7
ProgressiveRadixsortMSDPureCostModel=8

#Setting Values for Different Column Distributions
RandomDist = 1
SkewedDist = 2
SkyServerDist = 3
baseline_list = []
progressive_list = [ProgressiveQuicksort,ProgressiveBucketsortEquiheight,ProgressiveRadixsortLSD,ProgressiveRadixsortMSDPure]
Progressive_cm_list = [ProgressiveQuicksortCostModel,ProgressiveRadixsortMSDPureCostModel, ProgressiveRadixsortLSDCostModel, ProgressiveBucketsortEquiheightCostModel]
syntethical_workload_list = [Random,SeqOver,Skew]

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

def generate_column(column_dist,column_size):
    COLUMN_PATH = column_path(column_size)+'column'
    print("Generating Column")
    codestr = "./generate_column --column-size=" + str(column_size) + " --column-path=" + str(COLUMN_PATH)+ " --column-dist=" + str(column_dist)
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

def get_converged(stderr):
    import re
    match = re.search("Converged on query (\d+)", stderr)
    if match == None:
        return None
    return int(match.groups()[0])

def run_process(codestr):
    import subprocess
    p = subprocess.Popen(codestr.split(' '), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout,stderr = p.communicate()
    CONVERGED = get_converged(stderr)
    return (stdout, CONVERGED)


def run_experiment_baseline(COLUMN_SIZE,QUERY_PATTERN,QUERY_SELECTIVITY,ALGORITHM,NUM_QUERIES,COLUMN_DISTRIBUTION):
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
    (result,CONVERGED) = run_process(codestr)
    cursor.execute('''INSERT INTO experiments(algorithm_id, workload_id, column_size, query_selectivity,column_distribution_id, converged)
                  VALUES(:algorithm_id,:workload_id, :column_size, :query_selectivity, :column_distribution_id, :converged)''',
                  {'algorithm_id':ALGORITHM, 'workload_id':QUERY_PATTERN, 'column_size':COLUMN_SIZE, 'query_selectivity':QUERY_SELECTIVITY, 'column_distribution_id':COLUMN_DISTRIBUTION, 'converged': CONVERGED})
    experiment_id =  cursor.execute('''SELECT id FROM experiments where algorithm_id = (?) and workload_id=(?) and column_size=(?) and query_selectivity=(?) and column_distribution_id=(?)
                    ''', (ALGORITHM,QUERY_PATTERN,COLUMN_SIZE,QUERY_SELECTIVITY,COLUMN_DISTRIBUTION))
    experiment_id = cursor.fetchone()
    experiment_id = experiment_id[0]
    result = result.split("\n")
    for query_number in range(0, len(result)-1):
        query_result = result[query_number].split(";")
        cursor.execute('''INSERT INTO queries(experiment_id, query_number, query_time,indexing_time,total_time,pref_sum_total_time,cost_model_time)
              VALUES(:experiment_id,:query_number, :query_time, :indexing_time, :total_time,:pref_sum_total_time,:cost_model_time)''',
              {'experiment_id':experiment_id, 'query_number':query_number, 'query_time':query_result[1], 'indexing_time':query_result[2], 'total_time':query_result[3], 'pref_sum_total_time': query_result[4], 'cost_model_time': query_result[5]})

def run_experiment_progressive(COLUMN_SIZE,QUERY_PATTERN,QUERY_SELECTIVITY,ALGORITHM,NUM_QUERIES,FIXED_DELTA,COLUMN_DISTRIBUTION):
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
    (result,CONVERGED) = run_process(codestr)
    cursor.execute('''INSERT INTO experiments(algorithm_id, workload_id, column_size, query_selectivity,fixed_delta,column_distribution_id, converged)
                  VALUES(:algorithm_id,:workload_id, :column_size, :query_selectivity,:fixed_delta, :column_distribution_id, :converged)''',
                  {'algorithm_id':ALGORITHM, 'workload_id':QUERY_PATTERN, 'column_size':COLUMN_SIZE, 'query_selectivity':QUERY_SELECTIVITY, 'fixed_delta':FIXED_DELTA, 'column_distribution_id':COLUMN_DISTRIBUTION, 'converged': CONVERGED})
    experiment_id =  cursor.execute('''SELECT id FROM experiments where algorithm_id = (?) and workload_id=(?) and column_size=(?) and query_selectivity=(?) and fixed_delta=(?) and column_distribution_id=(?)
                    ''', (ALGORITHM,QUERY_PATTERN,COLUMN_SIZE,QUERY_SELECTIVITY,FIXED_DELTA,COLUMN_DISTRIBUTION))
    experiment_id = cursor.fetchone()
    experiment_id = experiment_id[0]
    result = result.split("\n")
    for query_number in range(0, len(result)-1):
        query_result = result[query_number].split(";")
        cursor.execute('''INSERT INTO queries(experiment_id, query_number, query_time,indexing_time,total_time,pref_sum_total_time,cost_model_time)
              VALUES(:experiment_id,:query_number, :query_time, :indexing_time, :total_time,:pref_sum_total_time,:cost_model_time)''',
              {'experiment_id':experiment_id, 'query_number':query_number, 'query_time':query_result[1], 'indexing_time':query_result[2], 'total_time':query_result[3],'pref_sum_total_time':query_result[4], 'cost_model_time': query_result[5]})

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
    (result,CONVERGED) = run_process(codestr)
    cursor.execute('''INSERT INTO experiments(algorithm_id, workload_id, column_size, query_selectivity,fixed_interactivity_threshold,column_distribution_id, converged)
                  VALUES(:algorithm_id,:workload_id, :column_size, :query_selectivity,:fixed_interactivity_threshold, :column_distribution_id, :converged)''',
                  {'algorithm_id':ALGORITHM, 'workload_id':QUERY_PATTERN, 'column_size':COLUMN_SIZE, 'query_selectivity':QUERY_SELECTIVITY, 'fixed_interactivity_threshold':FIXED_INTERACTIVITY_THRESHOLD, 'column_distribution_id':COLUMN_DISTRIBUTION, 'converged': CONVERGED})
    experiment_id =  cursor.execute('''SELECT id FROM experiments where algorithm_id = (?) and workload_id=(?) and column_size=(?) and query_selectivity=(?) and fixed_interactivity_threshold=(?) and column_distribution_id=(?)
                    ''', (ALGORITHM,QUERY_PATTERN,COLUMN_SIZE,QUERY_SELECTIVITY,FIXED_INTERACTIVITY_THRESHOLD,COLUMN_DISTRIBUTION))
    experiment_id = cursor.fetchone()
    experiment_id = experiment_id[0]
    result = result.split("\n")
    for query_number in range(0, len(result)-1):
        query_result = result[query_number].split(";")
        cursor.execute('''INSERT INTO queries(experiment_id, query_number, delta, query_time,indexing_time,total_time,pref_sum_total_time,cost_model_time)
              VALUES(:experiment_id,:query_number, :delta, :query_time, :indexing_time, :total_time, :pref_sum_total_time,:cost_model_time)''',
              {'experiment_id':experiment_id, 'query_number':query_number, 'delta':query_result[0], 'query_time':query_result[1], 'indexing_time':query_result[2], 'total_time':query_result[3], 'pref_sum_total_time': query_result[4], 'cost_model_time': query_result[5]})


def template_run(ALGORITHM_LIST,DELTA_LIST=0,COLUMN_SIZE_LIST=[100000],COLUMN_DISTRIBUTION_LIST=0,WORKLOAD_LIST=0,QUERY_SELECTIVITY_LIST=0,INTERACTIVITY_THRESHOLD_LIST=0,NUM_QUERIES=1000,INTERACTIVITY_IS_PERCENTAGE=1):
    generate_cost_model(100000000) #Mock Gen
    compile()
    if COLUMN_SIZE_LIST == 0:
        COLUMN_SIZE_LIST = [100000000]
    if WORKLOAD_LIST == 0:
        WORKLOAD_LIST = syntethical_workload_list
    if INTERACTIVITY_THRESHOLD_LIST == 0:
        INTERACTIVITY_THRESHOLD_LIST = [1.2]
    if DELTA_LIST == 0:
        DELTA_LIST = [0.005,0.01,0.05,0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0]
    if COLUMN_DISTRIBUTION_LIST == 0:
        COLUMN_DISTRIBUTION_LIST = [1]
    for column_dist in COLUMN_DISTRIBUTION_LIST:
        for column_size in COLUMN_SIZE_LIST:
            generate_cost_model(column_size) #Radix MSD Cost Model is dependent on column_size
            generate_column(column_dist,column_size)
            if column_size == 10000000:
                QUERY_SELECTIVITY_LIST = [0.00001,0.01]
            elif column_size == 100000000:
                QUERY_SELECTIVITY_LIST = [0.000001,0.01]
            elif column_size == 1000000000:
                if column_dist ==1 :
                    QUERY_SELECTIVITY_LIST = [0.1]
                else:
                    QUERY_SELECTIVITY_LIST = [0.01]
            else:
                QUERY_SELECTIVITY_LIST = [0.001]
            for query in WORKLOAD_LIST:
                for selectivity in QUERY_SELECTIVITY_LIST:
                    generate_workload(NUM_QUERIES,column_size,query,selectivity)
                    for algorithm in ALGORITHM_LIST:
                        if algorithm in baseline_list:
                            cursor.execute('''
                               SELECT id FROM experiments where algorithm_id = (?) and workload_id=(?) and column_size=(?) and query_selectivity=(?) and column_distribution_id=(?)
                            ''', (algorithm,query,column_size,selectivity,column_dist))
                            experiment_exists = cursor.fetchone()
                            if experiment_exists is None:
                                run_experiment_baseline(column_size,query,selectivity,algorithm,NUM_QUERIES,column_dist)
                            db.commit()
                        if algorithm in progressive_list:
                            for delta in DELTA_LIST:
                                cursor.execute('''
                                   SELECT id FROM experiments where algorithm_id = (?) and workload_id=(?) and column_size=(?) and query_selectivity=(?) and fixed_delta=(?) and column_distribution_id=(?)
                                ''', (algorithm,query,column_size,selectivity,delta,column_dist))
                                experiment_exists = cursor.fetchone()
                                if experiment_exists is None:
                                    run_experiment_progressive(column_size,query,selectivity,algorithm,NUM_QUERIES,delta,column_dist)
                                db.commit()
                        if algorithm in progressive_cm_list:
                            if INTERACTIVITY_IS_PERCENTAGE == 0:
                                # Query DB
                                cursor.execute('''
                                   SELECT min(total_time) from queries inner join experiments on (experiments.id = queries.experiment_id) where algorithm_id in (2,3,4,5,6) and column_size = (?) and workload_id = (?) and query_selectivity = (?) and query_number == 0 and column_distribution_id=(?)
                                ''', (column_size,query,selectivity,column_dist))
                                interactivity_threshold = cursor.fetchone()
                                cursor.execute('''
                                   SELECT id FROM experiments where algorithm_id = (?) and workload_id=(?) and column_size=(?) and query_selectivity=(?) and fixed_interactivity_threshold=(?) and column_distribution_id=(?)
                                ''', (algorithm,query,column_size,selectivity,interactivity_threshold[0],column_dist))
                                experiment_exists = cursor.fetchone()
                                if experiment_exists is None:
                                    run_experiment_cost_model(column_size,query,selectivity,algorithm,NUM_QUERIES,interactivity_threshold[0],column_dist,INTERACTIVITY_IS_PERCENTAGE)
                                db.commit()
                            else:
                                for interactivity_threshold in INTERACTIVITY_THRESHOLD_LIST:
                                    cursor.execute('''
                                       SELECT id FROM experiments where algorithm_id = (?) and workload_id=(?) and column_size=(?) and query_selectivity=(?) and fixed_interactivity_threshold=(?) and column_distribution_id=(?)
                                    ''', (algorithm,query,column_size,selectivity,interactivity_threshold,column_dist))
                                    experiment_exists = cursor.fetchone()
                                    if experiment_exists is None:
                                        run_experiment_cost_model(column_size,query,selectivity,algorithm,NUM_QUERIES,interactivity_threshold,column_dist,INTERACTIVITY_IS_PERCENTAGE)
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
        INTERACTIVITY_THRESHOLD_LIST = [1.2]
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
                        run_experiment_progressive(column_size,query,selectivity,algorithm,NUM_QUERIES,delta,SkyServerDist)
                    db.commit()
            if algorithm in progressive_cm_list:
                if QUERY_DECAY != 0:
                    cursor.execute('''
                           SELECT id FROM experiments where algorithm_id = (?) and workload_id=(?) and column_size=(?) and query_selectivity=(?) and fixed_interactivity_threshold=(?) and column_distribution_id=(?)
                        ''', (algorithm,query,column_size,selectivity,1.2,SkyServerDist))
                    experiment_exists = cursor.fetchone()
                    if experiment_exists is None:
                        run_experiment_cost_model(column_size,query,selectivity,algorithm,NUM_QUERIES,1.2,SkyServerDist,INTERACTIVITY_IS_PERCENTAGE,QUERY_DECAY)
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


def run_skyserver_progressive():
    ALGORITHM_LIST = progressive_list
    run_skyserver(ALGORITHM_LIST)




run_skyserver_progressive()

db.close()