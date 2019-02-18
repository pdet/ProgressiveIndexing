import os
import inspect

SCRIPT_PATH =  os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe()))) # script directory
os.chdir(SCRIPT_PATH)
os.chdir("..")
SkyServer=1

def compile():
    print("Compiling")
    os.environ['OPT'] = 'true'
    if os.system('cmake -DCMAKE_BUILD_TYPE=Release && make') != 0:
        print("Make Failed")
        exit()

def downloadSkyServer():
    if os.path.exists(SCRIPT_PATH+"real_data/skyserver") != 1:
        print("Create Item")

def run_skyserver():
    compile()
    # ALGORITHM_LIST = [FullIndex,StandardCracking,StochasticCracking,ProgressiveStochasticCracking,CoarseGranularIndex,ProgressiveQuicksort,ProgressiveQuicksortCostModel]
    COLUMN_PATH = "real_data/skyserver/skyserver.data"
    QUERY_PATH = "real_data/skyserver/query"
    ANSWER_PATH = "real_data/skyserver/answer"
    NUM_QUERIES = 158325
    COLUMN_SIZE = 585624220
    CORRECTNESS = 0
    QUERY_PATTERN = SkyServer
    QUERY_SELECTIVITY_LIST = [0.0000002,0.001,10,50]
    for query_sel in QUERY_SELECTIVITY_LIST:
        codestr = "./generate_workload --num-queries=" + str(NUM_QUERIES) + " --column-size=" + str(COLUMN_SIZE)  \
                  + " --column-path=" + str(COLUMN_PATH) + " --query-path=" + str(QUERY_PATH)+"_"+str(query_sel) + " --answer-path=" + str(ANSWER_PATH)+"_"+str(query_sel) + " --selectivity=" \
                  + str(query_sel) + " --queries-pattern=" +  str(QUERY_PATTERN)
    print (codestr)
    if os.system(codestr) != 0:
        print("Generating Queries Failed")
        exit()
    # for algorithm in ALGORITHM_LIST:
    #     codestr ="./main --num-queries=" + str(NUM_QUERIES) + " --column-size=" + str(COLUMN_SIZE) + \
    #          " --algorithm="+str(algorithm)+ " --column-path=" + str(COLUMN_PATH) + " --query-path=" \
    #          + str(QUERY_PATH) + " --answer-path=" + str(ANSWER_PATH) + " --correctness=" + str(CORRECTNESS)
    #     print(codestr)
    #     if CORRECTNESS:
    #         if os.system(codestr) != 0:
    #             print("Failed!")
    #     else:
    #         getFolderToSaveExperiments(algorithm,str(COLUMN_SIZE)+"_"+str(QUERY_PATTERN)+ "/")
    #         result = os.popen(codestr).read()
    #         file = create_output()
    #         generate_output(file,result,QUERY_PATTERN,algorithm)
run_skyserver()