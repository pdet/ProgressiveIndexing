
import sqlite3

dbs = ["results/results_baseline.db", "results/results_progressive_synthetic.db", "results/results_progressive_sky.db"]

distributions = ["Random", "Skew", "SkyServer"]
#workloads = ["Random", "SeqOver", "SeqRand", "ZoomIn", "SeqZoomIn", "Skew", "ZoomOutAlt", "Periodic", "ZoomInAlt"]

distributions = ["Random"]
workloads = ["Random"]
workload_results = dict()
for workload in workloads:
	workload_results[workload] = dict()

for db in dbs:
	print(db)
	c = sqlite3.connect(db).cursor()
	# figure out the set of algorithms
	c.execute("SELECT DISTINCT name FROM algorithms, experiments WHERE algorithms.id=algorithm_id;")
	algorithms = [x[0] for x in c.fetchall()]
	for algorithm_name in algorithms:
		c.execute("SELECT algorithms.id FROM algorithms WHERE name=?;", (algorithm_name,))
		algorithm_id = c.fetchall()[0][0]
		for workload_name in workloads:
			c.execute("SELECT workloads.id FROM workloads WHERE name=?;", (workload_name,))
			workload_id = c.fetchall()[0][0]
			for distribution_name in distributions:
				c.execute("SELECT column_distributions.id FROM column_distributions WHERE name=?;", (distribution_name,))
				distribution_id = c.fetchall()[0][0]

				print(algorithm_name, workload_name, distribution_name)
				c.execute("SELECT id FROM experiments WHERE workload_id=? AND algorithm_id=? AND column_distribution_id=? AND query_selectivity>0.001", (workload_id, algorithm_id, distribution_id))
				results = c.fetchall()
				if len(results) == 0:
					continue

				experiment_id = results[0][0]
				print(experiment_id)

				results = c.execute("SELECT total_time FROM queries WHERE query_number=0 AND experiment_id=?;", (experiment_id,)).fetchall()
				if len(results) == 0:
					print("?")
					continue

				first_query = results[0][0]

				print("First query", first_query)

				c.execute("SELECT SUM(total_time) FROM queries WHERE experiment_id=?", (experiment_id,))
				total_time = c.fetchall()[0][0]

				print("Total time", total_time)





"""\begin{table}[ht]
\centering\small
\begin{tabular}{l|l|l|l|l|l|l}
  & Workload & PI & AAI & CGI & PSC & STD \\
\hline
\multirow{9}{*}{\rotatebox[origin=c]{90}{Uniform Random}} & Random &  234& 201 & 388 & \cellcolor{green!25}165 & 2 \\
& Skew &  202 &200 & 550 & \cellcolor{green!25}166 & 3 \\
& SeqRandom &  1542 & 1431 & 1944 &\cellcolor{green!25}1399 & 4\\
& SeqZoomIn &  201 & 199 & 373 & \cellcolor{green!25}164 & 5 \\
& Periodic & 205 & 200& 387 & \cellcolor{green!25}165 & 6\\
& ZoomIn & 2029 & 1449 & 2377 & \cellcolor{green!25}1415 & 8\\
& Sequential & 199 & 199 & 389 & \cellcolor{green!25}164 & 10\\
& ZoomOutAlt & 238 & 200 &421 & \cellcolor{green!25}165 & 22\\
& ZoomInAlt & 221 & 200 & 387 & \cellcolor{green!25}164 & 33\\
 \hline
\multirow{9}{*}{\rotatebox[origin=c]{90}{Skewed}} & Random &  234& 201 & 388 & \cellcolor{green!25}165 & 2 \\
& Skew &  202 &200 & 550 & \cellcolor{green!25}166 & 3 \\
& SeqRandom &  1542 & 1431 & 1944 &\cellcolor{green!25}1399 & 4\\
& SeqZoomIn &  201 & 199 & 373 & \cellcolor{green!25}164 & 5 \\
& Periodic & 205 & 200& 387 & \cellcolor{green!25}165 & 6\\
& ZoomIn & 2029 & 1449 & 2377 & \cellcolor{green!25}1415 & 8\\
& Sequential & 199 & 199 & 389 & \cellcolor{green!25}164 & 10\\
& ZoomOutAlt & 238 & 200 &421 & \cellcolor{green!25}165 & 22\\
& ZoomInAlt & 221 & 200 & 387 & \cellcolor{green!25}164 & 33\\
\hline
&  SkyServer &  234& 201 & 388 & \cellcolor{green!25}165 & 2 \\
 \hline
\end{tabular}
\caption{Initial query cost}
\label{tbl:expsummary}
\end{table}"""