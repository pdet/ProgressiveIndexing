import os
import inspect
import sqlite3
import urllib

# script directory
SCRIPT_PATH = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
os.chdir(SCRIPT_PATH)


# Compilation
def clean_generated_data():
    os.system('rm -r generated_data')


def compile_pu():
    print("Compiling")
    os.environ['OPT'] = 'true'
    if os.system('cmake -DCMAKE_BUILD_TYPE=Release && make') != 0:
        print("Make Failed")
        exit()


compile_pu()

# Generate Columns
RANDOM = "1"
SKEWED = "2"


def column_path(column_size):
    path = os.path.join(SCRIPT_PATH, "generated_data", str(column_size))
    os.system('mkdir -p ' + path)
    return path + "/"


def generate_column(column_dist, column_size):
    COLUMN_PATH = column_path(column_size) + "column"
    print("Generating Column")
    GEN_COL_PATH = os.path.join(SCRIPT_PATH, "src", "input")
    codestr = GEN_COL_PATH+"/generate_column" + " --size=" + str(column_size) + " --path=" + str(
        COLUMN_PATH) + " --pattern=" + str(column_dist)
    print(codestr)
    os.system(codestr)


generate_column(1, 1000000)


# Generate Queries
def query_path(EXPERIMENT_PATH, SELECTIVITY_PERCENTAGE, QUERIES_PATTERN):
    return EXPERIMENT_PATH + "query_" + str(SELECTIVITY_PERCENTAGE) + "_" + str(QUERIES_PATTERN)


def answer_path(EXPERIMENT_PATH, SELECTIVITY_PERCENTAGE, QUERIES_PATTERN):
    return EXPERIMENT_PATH + "answer_" + str(SELECTIVITY_PERCENTAGE) + "_" + str(QUERIES_PATTERN)


def generate_workload(num_queries, column_size, query, selectivity):
    COLUMN_PATH = column_path(column_size)
    QUERY_PATH = query_path(COLUMN_PATH, selectivity, query)
    ANSWER_PATH = answer_path(COLUMN_PATH, selectivity, query)
    print("Generating Queries")
    codestr = "./generate_workload --num-queries=" + str(num_queries) + " --column-size=" + str(column_size) \
              + " --column-path=" + str(COLUMN_PATH) + "column" + " --query-path=" + str(
        QUERY_PATH) + " --answer-path=" + str(ANSWER_PATH) + " --selectivity=" \
              + str(selectivity) + " --queries-pattern=" + str(query)
    print(codestr)
    os.system(codestr)
# Generate Updates

# Run Algorithms
