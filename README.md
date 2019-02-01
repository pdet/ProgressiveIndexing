# Progressive Indexing
This project is a stand-alone implementation of all the experiments presented in the **Progressive Index: Indexing Without Prejudice** paper.

# Requirements
[CMake](https://cmake.org) to be installed and a `C++11` compliant compiler. Python 2.7 is necessary to run all the setup scripts. R ("dplyr", "ggplot2", "ggthemes", "ggrepel", "data.table", "RColorBrewer") is used to plot the results to the figures referenced in the paper.

## Available Indexing Algorithms
* [Standard Cracking](https://stratos.seas.harvard.edu/files/IKM_CIDR07.pdf)
* [Stochastic Cracking](http://www.cs.au.dk/~karras/StochasticDatabaseCracking.pdf)
* [Progressive Stochastic Cracking](http://www.cs.au.dk/~karras/StochasticDatabaseCracking.pdf)
* [Coarse Granular Index](www.vldb.org/pvldb/vol7/p97-schuhknecht.pdf)
* [Full Index (B+ Tree)](https://www.nowpublishers.com/article/Details/DBS-028)
* Progressive Quicksort Fixed Delta
* Progressive Quicksort Self-Adjusting Delta


## Available Index Update Algorithms
* [Merge Complete](https://stratos.seas.harvard.edu/publications/updating-cracked-database)
* [Merge Partial](https://stratos.seas.harvard.edu/publications/updating-cracked-database)
* [Merge Ripple](https://stratos.seas.harvard.edu/publications/updating-cracked-database)
* Progressive Mergesort

## Available Workloads


## Running/Plotting the experiments/results
To run all the experiments and automatically plot all the images used in the paper execute the run.py script. After it, all pictures will be generated under the generated_plots/ folder.
Note that the first time you run this application it might take a couple of hours since it download/generate all data and workloads used, as check all the algorithms for correctness before running and timing them.
```bash
python run.py
```

## Manually Running Experiments

## Third Party Code
* www.github.com/felix-halim/scrack
* www.bigdata.uni-saarland.de/publications/uncracked_pieces_sourcecode.zip