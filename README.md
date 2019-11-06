# Progressive Indexing ![Build](https://github.com/pdet/ProgressiveIndexing/workflows/CI/badge.svg)

This project is a stand-alone implementation of all the current progressive indexing algorithms.

# Requirements
[CMake](https://cmake.org) to be installed and a `C++11` compliant compiler. Python 2.7 is necessary to run the setup scripts for the Cost Model constants.

# Available Indexing Algorithms
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
The Sloan Digital Sky Survey is a project that maps the universe. The data set and interactive data exploration query logs are publicly available via the SkyServer website.
The data set contains almost 600 million tuples, with around 160,000 range queries that focus on specific sections of the domain before moving to different areas. 

## Synthetic 
The synthetic data set is composed of two data distributions, a random and a skewed one. They consist of n 8-byte integers distributed in the range of [0, n). All workloads consist of queries in the form 
```sql
SELECT SUM(R.A) FROM R WHERE R.A BETWEEN low AND high
```
The values for low and high are chosen based on the workload pattern. The different workload patterns and their mathematical description are originally from the [Stochastic Cracking](http://www.cs.au.dk/~karras/StochasticDatabaseCracking.pdf) paper.

# Running the experiments
The header for printed output:
delta(if progressive indexing);query processing time; index creation time ; total time; prefix sum; cost model cost

### Compile
First, we compile the code using release (-O3) mode
```bash
cmake -DCMAKE_BUILD_TYPE=Release && make
```

### Generate Constants
The Self-Adjusting Delta algorithms rely on constants hardware dependent (e.g., sequential scan, random writes, ...). Execute the following script to generate those constants before running said algorithms. 
```bash
python scripts/cost_model/generate_constants.py
```

### Generate Data
* SkyServer:
The skyserver data and workload can be downloaded [here](https://zenodo.org/record/2557531#.XHpgpZNKjUI) 

* Syntethical :
    * column-size is an integer representing the size of the column.
    * column-path is the path where the generated column will be stored.
    * column-dist is the distribution for the generated column
        * 1 = Random
        * 2 = Skewed
```bash
./generate_column --column-size=(?)   --column-path=(?) --column-path=(?) --column-dist=(?)
```
Example:
```bash
./generate_column --column-size=10000000 --column-path=col1 --column-dist=1
```

### Generate Workload
After generating the dataset, you can generate workload:
```bash
./generate_workload --num-queries=(?) --column-size=(?) --column-path=(?)  --query-path=(?) --answer-path=(?) --selectivity=(?) --queries-pattern=(?)
```
* num-queries is an integer representing the total amount of queries in the workload.
* column-size is an integer representing the size of the column.
* column-path is the path where the generated column is stored.
* query-path is the path where the generated queries will be stored.
* answer-path is the path where the generated query_answers will be stored. (This is used for correctness)
* selectivity is a float number representing the per-query selectivity. (10.0 means that every query has 10% of the whole domain [0,n) as their result])
* queries-pattern is the pattern the queries will follow.
     * 2 = Random
     * 3 = SeqOver
     * 4 = SeqRand
     * 5 = ZoomIn
     * 6 = SeqZoomIn
     * 7 = Skew
     * 8 = ZoomOutAlt
     * 9 = Periodic
     * 10 = ZoomInAlt
Example:
```bash
./generate_workload --num-queries=10000 --column-size=10000000 --column-path=col1 --query-path=q1 --answer-path=a1 --selectivity=0.01 --queries-pattern=2
```

### Run Experiments
After generating the data and the workload, you can run any of the implemented algorithms.:
All algorithms have the following parameters:
* num-queries is an integer representing the total amount of queries in the workload.
* column-size is an integer representing the size of the column.
* column-path is the path where the generated column is stored.
* query-path is the path where the generated queries are stored.
* answer-path is the path where the generated query_answers are stored.
* algorithm is the id of the to-be-executed algorithm.

#### For Progressive with fixed delta:
```bash
./main --num-queries=(?) --column-size=(?) --column-path=(?)  --query-path=(?) --answer-path=(?)  --algorithm=(?) --delta=(?)
```
* delta is a floating-point number used as the fixed delta for the entire workload.
* algorithm
    * 1 = Progressive Quicksort
    * 2 = Progressive Bucketsort Equiheight
    * 3 = Progressive Radixsort LSD
    * 4 = Progressive Radixsort MSD
Example:
```bash
./main --num-queries=10000 --column-size=10000000 --algorithm=13 --column-path=col1 --query-path=q1 --answer-path=a1 --delta=0.2
```
#### For Progressive with self-adjusting delta:
```bash
./main --num-queries=(?) --column-size=(?)--column-path=(?) --query-path=(?) --answer-path=(?) --algorithm=(?) --interactivity-threshold=(?) --interactivity-is-percentage=(?)  
```
* interactivity-threshold is a float representing the fixed interactivity threshold
* interactivity-is-percentage is an integer representing if the interactivity threshold is a percentage of the full scan cost or time in seconds.
* algorithm
    * 5 = Progressive Quicksort Cost Model
    * 6 = Progressive Bucketsort Equiheight Cost Model
    * 7 = Progressive Radixsort LSD Cost Model
    * 8 = Progressive Radixsort MSD Cost Model

Example with fixed interactivity:
```bash
./main --num-queries=158325 --column-size=585624220 --algorithm=8 --column-path=real_data/skyserver/skyserver.data --query-path=real_data/skyserver/query_0.1 --answer-path=real_data/skyserver/answer_0.1 --interactivity-threshold=0.8 --interactivity-is-percentage=1
```

# Third-Party Code
* Syntethical Workloads: www.github.com/felix-halim/scrack

# Papers
* [Progressive Indices: Indexing for Interactive Data Analysis @ VLDB 2020](https://pdet.github.io/assets/papers/progidx_vldb.pdf)
* [Progressive Indices: Indexing without prejudice (Ph.D. Worskhop @ VLDB 2019)](http://ceur-ws.org/Vol-2175/paper11.pdf)
