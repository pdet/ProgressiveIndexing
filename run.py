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
ProgressiveBucketsortEquiheight=8
ProgressiveBucketsortEquiheightCostModel=9
ProgressiveRadixsortLSD=10
ProgressiveRadixsortLSDCostModel=11
ProgressiveRadixsortMSD=12
ProgressiveRadixsortMSDCostModel=13

COLUMN_SIZE_LIST = [100000000,1000000000,10000000000]

SWAP_PG_CRACKING_LIST = [0.1]
ALL_WORKLOAD_LIST = [Random,SeqOver,SeqInv,SeqRand,SeqNoOver,SeqAlt,ConsRandom,ZoomIn,ZoomOut,SeqZoomIn,SeqZoomOut,Skew,
                     ZoomOutAlt,SkewZoomOutAlt,Periodic,Mixed]

NUM_QUERIES = 10000
QUERY_SELECTIVITY_LIST = [0.001,0.0001,0.00001]

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
    if alg == ProgressiveBucketsortEquiheight:
        return 'pbs'
    if alg == ProgressiveBucketsortEquiheightCostModel:
        return 'pbscm'
    if alg == ProgressiveRadixsortLSD:
        return 'prlsd'
    if alg == ProgressiveRadixsortLSDCostModel:
        return 'prlsdcm'
    if alg == ProgressiveRadixsortMSD:
        return 'prmsd'
    if alg == ProgressiveRadixsortMSDCostModel:
        return 'prmsdcm'
    return alg

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

def getFolderToSaveExperiments(algorithm,folder=""):
    global PATH
    if os.path.exists("ResultsCSV/"+folder+translate_alg(algorithm) + "/") != 1:
        os.system('mkdir -p ResultsCSV/'+folder+translate_alg(algorithm) + "/")
    PATH = "ResultsCSV/"+ folder+translate_alg(algorithm) + "/"
    os.system ('rm -r '+PATH)
    os.system('mkdir -p ' + PATH)

#Output is a csv file with:
# ""algorithm;query_number;query_pattern;delta;query_time"
def generate_output(file,query_result,QUERY_PATTERN,algorithm):
    query_result = query_result.split("\n")
    for query in range(0, len(query_result)):
        file.write(translate_alg(algorithm) + ";" + str(query) + ";" + str(QUERY_PATTERN) + ';' + query_result[query])
        file.write('\n')
    file.close()

# Saving Experiments
def create_output():
    header = "algorithm;query_number;query_pattern;delta;query_time"
    file = open(PATH + "results.csv", "w")
    file.write(header)
    file.write('\n')
    return file

def compile():
    print("Compiling")
    os.environ['OPT'] = 'true'
    if os.system('cmake -DCMAKE_BUILD_TYPE=Release && make') != 0:
        print("Make Failed")
        exit()

def generate_cost_model():
    print("Generating Cost Model Constants")
    os.system("python scripts/cost_model/generate_constants.py")  

def setup():
    generate_cost_model()
    compile()
    for column_size in COLUMN_SIZE_LIST:
        generate_column(column_size)
    for column_size in COLUMN_SIZE_LIST:
        for query in ALL_WORKLOAD_LIST:
            for selectivity in QUERY_SELECTIVITY_LIST:
                generate_workload(NUM_QUERIES,column_size,query,selectivity)

def run_experiment(COLUMN_SIZE,QUERY_PATTERN,QUERY_SELECTIVITY,ALGORITHM,NUM_QUERIES,CORRECTNESS=0):
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
        getFolderToSaveExperiments(ALGORITHM,str(COLUMN_SIZE)+"_"+str(QUERY_PATTERN)+"_"+str(QUERY_SELECTIVITY) +"/")
        result = os.popen(codestr).read()
        file = create_output()
        generate_output(file,result,QUERY_PATTERN,ALGORITHM)

def run_all_workloads(ALGORITHM,CORRECTNESS=0):
    # ALL_WORKLOAD_LIST = [Random]
    for column_size in COLUMN_SIZE_LIST:
        for query in ALL_WORKLOAD_LIST:
            for selectivity in QUERY_SELECTIVITY_LIST:
                run_experiment(column_size,query,selectivity,ALGORITHM,NUM_QUERIES,CORRECTNESS)


def test_correctness():
    setup()
    ALGORITHM_LIST = [FullScan,FullIndex,StandardCracking,StochasticCracking,ProgressiveStochasticCracking,CoarseGranularIndex,ProgressiveQuicksort,ProgressiveQuicksortCostModel]
    for algorithm in ALGORITHM_LIST:
        run_all_workloads(algorithm,1)


def downloadSkyServer():
    if os.path.exists(SCRIPT_PATH+"real_data/skyserver") != 1:
        print("Create Item")

def run_skyserver():
    setup()
    ALGORITHM_LIST = [FullIndex,StandardCracking,StochasticCracking,ProgressiveStochasticCracking,CoarseGranularIndex,ProgressiveQuicksort,ProgressiveQuicksortCostModel]
    COLUMN_PATH = "real_data/skyserver/skyserver.data"
    QUERY_PATH = "real_data/skyserver/query"
    ANSWER_PATH = "real_data/skyserver/answer"
    NUM_QUERIES = 158325
    COLUMN_SIZE = 585624220
    CORRECTNESS = 0
    QUERY_PATTERN = SkyServer
    QUERY_SELECTIVITY = 0.001
    codestr = "./generate_workload --num-queries=" + str(NUM_QUERIES) + " --column-size=" + str(COLUMN_SIZE)  \
              + " --column-path=" + str(COLUMN_PATH) + " --query-path=" + str(QUERY_PATH) + " --answer-path=" + str(ANSWER_PATH) + " --selectivity=" \
              + str(QUERY_SELECTIVITY) + " --queries-pattern=" +  str(QUERY_PATTERN)
    print (codestr)
    if os.system(codestr) != 0:
        print("Generating Queries Failed")
        exit()
    for algorithm in ALGORITHM_LIST:
        codestr ="./main --num-queries=" + str(NUM_QUERIES) + " --column-size=" + str(COLUMN_SIZE) + \
             " --algorithm="+str(algorithm)+ " --column-path=" + str(COLUMN_PATH) + " --query-path=" \
             + str(QUERY_PATH) + " --answer-path=" + str(ANSWER_PATH) + " --correctness=" + str(CORRECTNESS)
        print(codestr)
        if CORRECTNESS:
            if os.system(codestr) != 0:
                print("Failed!")
        else:
            getFolderToSaveExperiments(algorithm,str(COLUMN_SIZE)+"_"+str(QUERY_PATTERN)+ "/")
            result = os.popen(codestr).read()
            file = create_output()
            generate_output(file,result,QUERY_PATTERN,algorithm)
def run():
    setup()
    # ALGORITHM_LIST = [FullScan,FullIndex,StandardCracking,StochasticCracking,ProgressiveStochasticCracking,CoarseGranularIndex,ProgressiveQuicksort,ProgressiveQuicksortCostModel]
    ALGORITHM_LIST = [FullIndex]
    for algorithm in ALGORITHM_LIST:
        run_all_workloads(algorithm)

def download_results():
    codestr = "rm -r -f ResultsCSV"
    os.system(codestr)
    FROM = "stones04:/export/scratch1/home/holanda/ProgressiveIndexing/ResultsCSV"
    TO = SCRIPT_PATH
    codestr = "scp -r " + FROM + " " + TO
    os.system(codestr)

def plots():
    os.system("python "+SCRIPT_PATH+"/scripts/plots/plot.py")

def generate_workload_csvs():
    compile()
    COLUMN_SIZE = 100000000
    ALL_WORKLOAD_LIST = [Random,SeqOver,SeqInv,SeqRand,SeqNoOver,SeqAlt,ConsRandom,ZoomIn,ZoomOut,SeqZoomIn,SeqZoomOut,Skew,
                         ZoomOutAlt,SkewZoomOutAlt,Periodic,Mixed]
    NUM_QUERIES = 10000
    SELECTIVITY_PERCENTAGE = 0.001
    os.system('rm -r '+ SCRIPT_PATH + "/worload_plots" )
    os.system('mkdir -p '+ SCRIPT_PATH + "/worload_plots")
    for workload in ALL_WORKLOAD_LIST:
        COLUMN_PATH = column_path(COLUMN_SIZE)
        QUERY_PATH = query_path(COLUMN_PATH,SELECTIVITY_PERCENTAGE,workload)
        ANSWER_PATH = answer_path(COLUMN_PATH,SELECTIVITY_PERCENTAGE,workload)
        codestr = "./generate_workload --num-queries=" + str(NUM_QUERIES) + " --column-size=" + str(COLUMN_SIZE)  \
                  + " --column-path=" + str(COLUMN_PATH)+ "column" + " --query-path=" + str(QUERY_PATH) + " --answer-path=" + str(ANSWER_PATH) + " --selectivity=" \
                  + str(SELECTIVITY_PERCENTAGE) + " --queries-pattern=" +  str(workload)
        result = os.popen(codestr).read()
        file = open(SCRIPT_PATH + "/worload_plots/" +str(workload) + ".csv", "w")
        file.write(result)
        file.close()

def template_correctness():
    ALGORITHM_LIST = [ProgressiveQuicksort,ProgressiveQuicksortCostModel,ProgressiveBucketsortEquiheight,
                      ProgressiveBucketsortEquiheightCostModel,ProgressiveRadixsortLSD,ProgressiveRadixsortLSDCostModel,
                      ProgressiveRadixsortMSD,ProgressiveRadixsortMSDCostModel]
    COLUMN_SIZE_LIST = [100000000]
    ALL_WORKLOAD_LIST = [Random]
    QUERY_SELECTIVITY_LIST = [0.001]
    num_queries = 10000
    clean_generated_data()
    generate_cost_model()
    compile()
    for column_size in COLUMN_SIZE_LIST:
        generate_column(column_size)
        for query in ALL_WORKLOAD_LIST:
            for selectivity in QUERY_SELECTIVITY_LIST:
                generate_workload(num_queries,column_size,query,selectivity)
                for algorithm in ALGORITHM_LIST:
                    run_experiment(column_size,query,selectivity,algorithm,num_queries,1)

def template_run():
    ALGORITHM_LIST = [ProgressiveQuicksort,ProgressiveQuicksortCostModel,ProgressiveBucketsortEquiheight,
                      ProgressiveBucketsortEquiheightCostModel,ProgressiveRadixsortLSD,ProgressiveRadixsortLSDCostModel,
                      ProgressiveRadixsortMSD,ProgressiveRadixsortMSDCostModel]
    COLUMN_SIZE_LIST = [100000000]
    ALL_WORKLOAD_LIST = [Random]
    QUERY_SELECTIVITY_LIST = [0.001]
    num_queries = 10000
    clean_generated_data()
    generate_cost_model()
    compile()
    for column_size in COLUMN_SIZE_LIST:
        generate_column(column_size)
        for query in ALL_WORKLOAD_LIST:
            for selectivity in QUERY_SELECTIVITY_LIST:
                generate_workload(num_queries,column_size,query,selectivity)
                for algorithm in ALGORITHM_LIST:
                    run_experiment(column_size,query,selectivity,algorithm,num_queries,0)

# test_correctness()
# test_correctness_updates()
# run()
# run_updates()
# run_skyserver()
# download_results()
# plots()
# generate_workload_csvs()

template_run()