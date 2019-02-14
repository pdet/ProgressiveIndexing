# Progressive Indexing
This project is a stand-alone implementation of all the current progressive indexing implementations.

# Requirements
[CMake](https://cmake.org) to be installed and a `C++11` compliant compiler. Python 2.7 is necessary to run all the setup scripts.
R ("dplyr", "ggplot2", "ggthemes", "ggrepel", "data.table", "RColorBrewer") is used to plot the results to the figures referenced in all the paper.

# Available Indexing Algorithms
* [Standard Cracking](https://stratos.seas.harvard.edu/files/IKM_CIDR07.pdf)
* [Stochastic Cracking](http://www.cs.au.dk/~karras/StochasticDatabaseCracking.pdf)
* [Progressive Stochastic Cracking](http://www.cs.au.dk/~karras/StochasticDatabaseCracking.pdf)
* [Coarse Granular Index](www.vldb.org/pvldb/vol7/p97-schuhknecht.pdf)
* [Full Index (B+ Tree)](https://www.nowpublishers.com/article/Details/DBS-028)
* Progressive Quicksort Fixed Delta
* Progressive Quicksort Self-Adjusting Delta
* Progressive Radixsort (MSD) Fixed Delta
* Progressive Radixsort (MSD) Self-Adjusting Delta
* Progressive Radixsort (LSD) Fixed Delta
* Progressive Radixsort (LSD) Self-Adjusting Delta
* Progressive Bucketsort (Equi-Height) Fixed Delta
* Progressive Bucketsort (Equi-Height) Self-Adjusting Delta

# Available Workloads
tbd

# Running the experiments
## Initializing SQLite
We use SQLite to store all the experiments. Be sure to initialize the database by executing the following script before running any experiments.
```bash
python scripts/sqlite.py
```

## Running Syntethical Experiments

To run all the experiments and automatically plot all the images used in the paper execute the run.py script. After it, all pictures will be generated under the generated_plots/ folder.
Note that the first time you run this application it might take a couple of hours since it download/generate all data and workloads used, as check all the algorithms for correctness before running and timing them.
```bash
python scripts/run_syntethic.py
```

## Running Real Experiments (AKA SkyServer Data/Workload)
```bash
python scripts/run_skyserver.py
```
## Running Experiments Specifying the parameters (Not Stored in SQLite)
tbd

## Checking for correctness
Check if all algorithms produce correct results for given experiment parameters.
```bash
python scripts/run_correctness.py
```

# Plotting the Results
tbd

## Downloading and Plotting the sqlite database that is the result of this project
tbd

# Third Party Code
* www.github.com/felix-halim/scrack
* www.bigdata.uni-saarland.de/publications/uncracked_pieces_sourcecode.zip

# Papers
* [Progressive Indexing : Indexing without prejudice (Arxiv)](https://www.nowpublishers.com/article/Details/DBS-028)
* [Progressive Indices : Indexing without prejudice (PhD Worskhop@VLDB)](http://ceur-ws.org/Vol-2175/paper11.pdf)