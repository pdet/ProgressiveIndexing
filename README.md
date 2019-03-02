# Progressive Indexing
This project is a stand-alone implementation of all the current progressive indexing implementations.

# Requirements
[CMake](https://cmake.org) to be installed and a `C++11` compliant compiler. Python 2.7 (with sqlite3 package) is necessary to run all the setup scripts.
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
## SkyServer

## Synthetic 

# Running the experiments

## Automatic
 To automatically run all the experiments you need to run two different scripts.
(1) The first script, will run all the experiments that compare the indexing algorithms. Will also populate a sqlite database (Called results.db) with all the results of the experiment. It also produces checkpoints, so its possible to resume runs. The script automatically downloads the SkySever data and generates and synthetical data that might be necessary.
```bash
python scripts/run_experiments.py
```
(2) The second script, generates tsv files, that are used to compared cost models and measured times. It only works for Progressive Indexing with self-adjusting delta.
```bash
python scripts/run_costmodels.py
```
## Manual
Its also possible to manually run any algorithm with any data distribution/size/workload/selectivity by issuing the follow manual comands, the results are printed in the console after execution using the following csv header:
delta(if progressive indexing);query processing time; index creation time ; total time; prefix sum
### Compile
First we compile the code using release (-O3) mode
```bash
cmake -DCMAKE_BUILD_TYPE=Release
```
```bash
make
```
### Generate Data
* SkyServer 
The skyserver data and workload can be manuall downloaded [here](https://zenodo.org/record/2557531#.XHpgpZNKjUI)  
* Syntethical :
```bash
./generate_column --column-size= column_size_integer   --column-path= path_to_store_column
```

### Generate Workload
After generating the dataset you can generate workload for it using the following command:
```bash
./generate_workload --num-queries=  num_queries_integer --column-size= column_size_integer --column-path= path_where_column_is_stored  --query-path= path_to_store_query --answer-path= path_to_store_query_anwers --selectivity= selectivity_double(100 = whole dataset) --queries-pattern= workload_id
```
Possible workload ids:
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

### Run Experiments
Finally, after generating the data and the workload, you can run any of the implemented algorithms.:
* For Baseline Algorithms:
```bash
./main --num-queries= number_queries_integer --column-size= column_size_integer --column-path= path_where_column_is_stored  --query-path= path_where_query_is_stored --answer-path= path_where_query_answer_is_stored   --algorithm=algorithm_id
```
Possible algorithm ids:
Full Scan = 1
Full Index = 2
Standard Cracking = 3
Stochastic Cracking = 4
Progressive Stochastic Cracking=5
Coarse Granular Index=6

* For Progressive with fixed delta:
```bash
./main --num-queries= number_queries_integer --column-size= column_size_integer --column-path= path_where_column_is_stored  --query-path= path_where_query_is_stored --answer-path= path_where_query_answer_is_stored   --algorithm=algorithm_id --delta= fixed_delta_double
```
Possible algorithm ids:
Progressive Quicksort=7
Progressive Bucketsort Equiheight=9
Progressive Radixsort LSD=11
Progressive Radixsort MSD=13

* For Progressive with self-adjusting delta:
```bash
./main --num-queries= number_queries_integer --column-size= column_size_integer --column-path= path_where_column_is_stored  --query-path= path_where_query_is_stored --answer-path= path_where_query_answer_is_stored   --algorithm= algorithm_id --interactivity-threshold= double_second_or_full_scan_percentage --interactivity-is-percentage=1_if_is_percentage_0_if_is_seconds
```
Possible algorithm ids:
Progressive Quicksort Cost Model=8
Progressive Bucketsort Equiheight Cost Model=10
Progressive Radixsort LSD Cost Model=12
Progressive Radixsort MSD Cost Model=14

## Checking for Correctness (In case you change any of the algorithms and wants to check if they are correct.)
Check if all algorithms produce correct results for given experiment parameters. By default is uses a 10^8 column and test all the syntethical workloads with 0.001 selectivity and 10k queries.
```bash
python scripts/run_correctness.py
```
# Analyzing the results
## Regular Runs

## Cost Model Vs Measured Time

# Third Party Code
* www.github.com/felix-halim/scrack
* www.bigdata.uni-saarland.de/publications/uncracked_pieces_sourcecode.zip

# Papers
* [Progressive Indices : Indexing without prejudice (PhD Worskhop@VLDB)](http://ceur-ws.org/Vol-2175/paper11.pdf)