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

# download_results()
# plots()
# generate_workload_csvs()