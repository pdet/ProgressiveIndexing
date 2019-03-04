# Progressive Indexing
This project is a stand-alone implementation of all the current progressive indexing implementations.

# Requirements
[CMake](https://cmake.org) to be installed and a `C++11` compliant compiler. Python 2.7 (with the sqlite3 package) is necessary to run all the setup scripts.
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
There are two types of workloads available. Both are based on the work of [Halim et al.](http://www.cs.au.dk/~karras/StochasticDatabaseCracking.pdf)
## SkyServer
The Sloan Digital Sky Survey is a project that maps the universe. The data set and interactive data exploration query logs are publicly available via the SkyServer website. This benchmark is focused on the range queries that are applied on the Right Ascension column of the PhotoObjAll table. The data set contains almost 600 million tuples, with around 160,000 range queries that focus on specific sections of the domain before moving to different areas. The data distribution and the workload distribution are shown below:
<img src="https://github.com/pholanda/ProgressiveIndexing/blob/master/Images/skyserver.png" width="900" height="300" />

## Synthetic 
The synthetic data set is composed of two data distributions, consisting of size n 8-byte integers distributed in the range of [0, n). We use two different data sets. The first one is composed of unique integers uniformly distributed, while the second one follows a skewed distribution with non-unique integers where 90% of the data is concentrated in the middle of the [0, n) range. The synthetic workload consists of 10^5 queries in the form 
```sql
SELECT SUM(R.A) FROM R WHERE R.A BETWEEN low AND high
```
The values for low and high are chosen based on the workload pattern. The different workload patterns and their mathematical description are depicted in the figure below, originally from the [Stochastic Cracking](http://www.cs.au.dk/~karras/StochasticDatabaseCracking.pdf) paper.
![alt text](Images/Syn-work.png)

# Running the experiments

## Automatic
 To automatically run all the experiments you need to run the following script:
```bash
python scripts/run_experiments.py
```
It will run all the experiments that compare the indexing algorithms through a variety of data sizes, data distributions, workloads, and query selectivities. In the case of progressive indexing will also use a variety of Interactivity Thresholds and Deltas. Will also populate a SQLite database (Called results.db) with all the results of the experiment. It also produces checkpoints, making it possible to resume runs. The script also automatically downloads the SkySever data/workload from an online repository and generates and synthetical data/workload that might be necessary. Note that a full run might take many hours.

## Manual
It's also possible to manually run any algorithm with any data distribution/size/workload/selectivity by issuing the following manual commands, the results are printed in the console in this order:
delta(if progressive indexing);query processing time; index creation time ; total time; prefix sum; cost model cost

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
* SkyServer=1
* Random=2
* SeqOver=3
* SeqRand=4
* ZoomIn=5
* SeqZoomIn=6
* Skew=7
* ZoomOutAlt=8
* Periodic=9
* ZoomInAlt=10

### Run Experiments
Finally, after generating the data and the workload, you can run any of the implemented algorithms.:
#### For Baseline Algorithms:
```bash
./main --num-queries= number_queries_integer --column-size= column_size_integer --column-path= path_where_column_is_stored  --query-path= path_where_query_is_stored --answer-path= path_where_query_answer_is_stored   --algorithm=algorithm_id
```
Possible algorithm ids:
* Full Scan = 1
* Full Index = 2
* Standard Cracking = 3
* Stochastic Cracking = 4
* Progressive Stochastic Cracking=5
* Coarse Granular Index=6

#### For Progressive with fixed delta:
```bash
./main --num-queries= number_queries_integer --column-size= column_size_integer --column-path= path_where_column_is_stored  --query-path= path_where_query_is_stored --answer-path= path_where_query_answer_is_stored   --algorithm=algorithm_id --delta= fixed_delta_double
```
Possible algorithm ids:
* Progressive Quicksort=7
* Progressive Bucketsort Equiheight=9
* Progressive Radixsort LSD=11
* Progressive Radixsort MSD=13

#### For Progressive with self-adjusting delta:
```bash
./main --num-queries= number_queries_integer --column-size= column_size_integer --column-path= path_where_column_is_stored  --query-path= path_where_query_is_stored --answer-path= path_where_query_answer_is_stored   --algorithm= algorithm_id --interactivity-threshold= double_second_or_full_scan_percentage --interactivity-is-percentage=1_if_is_percentage_0_if_is_seconds
```
Possible algorithm ids:
* Progressive Quicksort Cost Model=8
* Progressive Bucketsort Equiheight Cost Model=10
* Progressive Radixsort LSD Cost Model=12
* Progressive Radixsort MSD Cost Model=14

## Checking for Correctness
Check if all algorithms produce correct results for given experiment parameters. By default, it uses a 10^8 column, uniform distribution, and test all the synthetical workload patterns with 0.001 selectivity and 10k queries.
```bash
python scripts/run_correctness.py
```
# Analyzing the results
If the results are generated by the experiments script (scripts/run_experiments.py), all the results are stored in an SQLite instance (results.db) in the repo root. It can be queried/exported to python/r for plotting purposes. Below you can see the SQLite schema used to store the experiment results.
![alt text](Images/sqliteschema.png)

# Third Party Code
* www.github.com/felix-halim/scrack
* www.bigdata.uni-saarland.de/publications/uncracked_pieces_sourcecode.zip

# Papers
* [Progressive Indices : Indexing without prejudice (PhD Worskhop@VLDB)](http://ceur-ws.org/Vol-2175/paper11.pdf)