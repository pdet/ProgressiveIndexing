
import sqlite3
import numpy
import math


dbs = ['results2/results_adaptive.db','results2/results_fixeddeltas.db','results2/results_pb.db','results2/results_pq.db','results2/results_prl.db','results2/results_prm.db','results2/results_skyserver.db']

distributions = ["Random", "Skew", "SkyServer"]
workloads = ["SkyServer", "Random", "SeqOver", "SeqRand", "ZoomIn", "SeqZoomIn", "Skew", "ZoomOutAlt", "Periodic", "ZoomInAlt"]
# distributions = ["Random"]
algorithms = ["ProgressiveQuicksortCostModel", "ProgressiveBucketsortEquiheightCostModel", "ProgressiveRadixsortLSDCostModel", "ProgressiveRadixsortMSDCostModel", 'AdaptiveAdaptiveIndexing']
# workloads = ["Random", "SeqOver", "Skew", "ZoomOutAlt", "Periodic", "ZoomInAlt"]

# distributions = ["Random", "SkyServer"]
# workloads = ["Periodic"]

# for db in dbs:
# 	print(db)
# 	c = sqlite3.connect(db).cursor()
# 	# figure out the set of algorithms
# 	c.execute("SELECT DISTINCT name FROM algorithms, experiments WHERE algorithms.id=algorithm_id and algorithms.id > 6 and algorithms.id<15;")
# 	algorithms = [x[0] for x in c.fetchall()]
# 	for distribution_name in distributions:
# 		c.execute("SELECT column_distributions.id FROM column_distributions WHERE name=?;", (distribution_name,))
# 		distribution_id = c.fetchall()[0][0]

# 		for workload_name in workloads:
# 			c.execute("SELECT workloads.id FROM workloads WHERE name=?;", (workload_name,))
# 			workload_id = c.fetchall()[0][0]
# 			print('')
# 			print("&"+workload_name + "&"),
# 			for algorithm_name in algorithms:
# 				c.execute("SELECT algorithms.id FROM algorithms WHERE name=?;", (algorithm_name,))
# 				algorithm_id = c.fetchall()[0][0]
# 				c.execute("SELECT id FROM experiments WHERE workload_id=? AND algorithm_id=? AND column_distribution_id=? AND query_selectivity<0.001", (workload_id, algorithm_id, distribution_id))
# 				results = c.fetchall()
# 				if len(results) == 0:
# 					continue

# 				experiment_id = results[0][0]
# 				# print(experiment_id)
# 				# print(algorithm_name, workload_name, distribution_name)

# 				results = c.execute("SELECT total_time FROM queries WHERE query_number=0 AND experiment_id=?;", (experiment_id,)).fetchall()
# 				if len(results) == 0:
# 					print("?")
# 					continue

# 				first_query = results[0][0]

# 				# print("First query", first_query)

# 				c.execute("SELECT SUM(total_time) FROM queries WHERE experiment_id=?", (experiment_id,))
# 				total_time = c.fetchall()[0][0]

# 				c.execute("SELECT total_time FROM queries WHERE experiment_id=? and query_number = 9999", (experiment_id,))
# 				total_time = int(total_time + c.fetchall()[0][0] * (1000000 - 10000))
# 				print(str(total_time) + "&"),

result_dict = {}
for distribution_name in distributions:
	result_dict[distribution_name] = {}
	for algorithm_name in algorithms:
		result_dict[distribution_name][algorithm_name] = {}

for db in dbs:
	print(db)
	c = sqlite3.connect(db).cursor()
	# figure out the set of algorithms
	c.execute("SELECT DISTINCT name FROM algorithms, experiments WHERE algorithms.id=algorithm_id  and algorithms.id IN (8, 10, 12, 14, 15);")
	algos = [x[0] for x in c.fetchall()]
	for workload_name in workloads:
		c.execute("SELECT workloads.id FROM workloads WHERE name=?;", (workload_name,))
		workload_id = c.fetchall()[0][0]
		for distribution_name in distributions:
			c.execute("SELECT column_distributions.id FROM column_distributions WHERE name=?;", (distribution_name,))
			distribution_id = c.fetchall()[0][0]

			# full_index_results = c.execute("SELECT MAX(total_time) FROM queries, experiments, algorithms, workloads, column_distributions WHERE experiment_id=experiments.id AND algorithm_id=algorithms.id AND workload_id=workloads.id AND column_distributions.id=column_distribution_id AND algorithms.name='FullIndex' AND query_number>1 AND workloads.name=?", (workload_name,)).fetchall()
			# if len(full_index_results) == 0:
			# 	print(workload_name, distribution_name)
			# 	exit(1)
			for algorithm_name in algos:
				c.execute("SELECT algorithms.id FROM algorithms WHERE name=?;", (algorithm_name,))
				algorithm_id = c.fetchall()[0][0]
				c.execute("SELECT id FROM experiments WHERE workload_id=? AND algorithm_id=? AND column_distribution_id=? AND query_selectivity>0.001", (workload_id, algorithm_id, distribution_id))
				results = c.fetchall()
				if len(results) == 0:
					continue

				experiment_id = results[0][0]
				# print(experiment_id)

				results = c.execute("SELECT total_time FROM queries WHERE query_number=0 AND experiment_id=?;", (experiment_id,)).fetchall()
				if len(results) == 0:
					continue

				# print(algorithm_name)

				first_query = results[0][0]

				# print("First query", first_query)

				c.execute("SELECT SUM(total_time) FROM queries WHERE experiment_id=?", (experiment_id,))
				total_time = c.fetchall()[0][0]

				# c.execute("SELECT MIN(total_time) FROM queries WHERE experiment_id=?", (experiment_id,))
				# total_time = int(total_time_before + c.fetchall()[0][0] * (1000000.0 - 10000.0))
				# print(total_time_before, total_time)

				# print("Total time", total_time)

				# find robustness
				results = c.execute("SELECT query_number, total_time FROM queries WHERE experiment_id=? ORDER BY query_number", (experiment_id,)).fetchall()
				all_query_numbers = numpy.array([x[0] for x in results])
				all_total_times = numpy.array([x[1] for x in results])
				squared_error = 0

				query_numbers = all_query_numbers[:1000]
				total_times = all_total_times[:1000]

				z = numpy.polyfit(query_numbers, total_times, 2)
				poly = numpy.poly1d(z)
				predicted = poly(query_numbers)
				squared_error += numpy.sum((predicted - total_times) ** 2)

				result_dict[distribution_name][algorithm_name][workload_name] = [first_query, total_time, squared_error]


print(result_dict)


name_map = {"Random": "Uniform Random", "Skew": "Skewed", "SkyServer": "SkyServer"}

# now print the values
def render_table(render_index, cell_type, caption, label):
	print('')

	print("""
\\begin{table}[ht]
\\centering\small
\\begin{tabular}{l|l|l|l|l|l|l}
  & Workload & PQ & PB & PLSD & PMSD & AA \\\\""")
	for distribution in distributions:
		distribution_text = name_map[distribution]
		print('\\hline')
		wloads = result_dict[distribution][algorithms[0]].keys()

		for workload in wloads:
			line = ' & ' + workload
			if workload == wloads[0] and distribution != "SkyServer":
				line = "\\multirow{" + str(len(wloads)) + "}{*}{\\rotatebox[origin=c]{90}{" + distribution_text + "}}" + line
			smallest = None
			for algore in algorithms:
				if workload in result_dict[distribution][algore]:
					if smallest is None:
						smallest = algore
					elif result_dict[distribution][algore][workload][render_index] < result_dict[distribution][smallest][workload][render_index]:
						smallest = algore
			for algore in algorithms:
				if workload in result_dict[distribution][algore]:
					res = result_dict[distribution][algore][workload][render_index]
					line += "& "
					if algore == smallest:
						line += "\\cellcolor{green!25}"
					if cell_type == 'float':
						if res < 0.01:
							line += '{:0.1e}'.format(res)
						elif res < 1:
							line += "%.2f" % (res,)
						elif res < 100:
							line += "%.1f" % (res,)
						else:
							line += "%d" % (int(res),)

					else:
						line += "%d" % (res,)
				else:
					line += "& ?"
			line += " \\\\"

			print(line)
	print("""
\\end{tabular}
\\caption{CAPTION}
\\label{LABEL}
\\end{table}""".replace("CAPTION", caption).replace("LABEL", label))


print("\n\n-- FIRST QUERY--\n\n")
render_table(0, 'float', "First query cost", "tbl:firstquery")
print("\n\n-- TOTAL TIME--\n\n")
render_table(1, 'float', "Cumulative Time", "tbl:cumtime")
print("\n\n-- ROBUSTNESS--\n\n")
render_table(2, 'float', "Robustness", "tbl:robustness")

print("\n\n-- DONE--\n\n")



# & Skew &  202 &200 & 550 & \cellcolor{green!25}166  \\

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