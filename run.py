import os
import inspect
import sqlite3
import urllib

# script directory
SCRIPT_PATH = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
os.chdir(SCRIPT_PATH)

def compile():
    print("Compiling")
    os.environ['OPT'] = 'true'
    if os.system('cmake -DCMAKE_BUILD_TYPE=Debug && make') != 0:
        print("Make Failed")
        exit()

def run():
    os.system('src/PIMain >> result.csv')

# def plot():

compile()
run()