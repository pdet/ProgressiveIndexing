import os
import inspect
import sqlite3

SCRIPT_PATH =  os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe()))) # script directory
os.chdir(SCRIPT_PATH)
os.chdir("..")

experiments = ["stones01","skyserver_baseline", "skyserver_progressive_indexing", "all_fullscan"] 
machines = ["stones03","stones07","stones08","stones09" ]

def download_databases():
	for i in range(0,len(experiments)):
		os.system("scp -r "+machines[i]+":/export/scratch1/home/holanda/ProgressiveIndexing/results.db ./"+experiments[i]+".db")

def create_new_db():
	os.system("rm results.db")
	os.system("python scripts/sqlite.py")

def merge_databases():
	final_db = sqlite3.connect('results.db')
	final_cursor = final_db.cursor()
	for i in range(0,len(experiments)):
		current_db = sqlite3.connect(experiments[i]+'.db')
		current_cursor = current_db.cursor()
		current_cursor.execute('''SELECT * FROM experiment''')
		result_experiment= current_cursor.fetchall()
		for experiment_row in result_experiment:
			print(experiment_row)
			if experiment_row[5] is None and experiment_row[6] is not None:
				final_cursor.execute('''SELECT id FROM experiment where algorithm_id = (?) and workload_id=(?) and column_size=(?) and query_selectivity=(?) and fixed_interactivity_threshold=(?)''', (experiment_row[1],experiment_row[2],experiment_row[3],experiment_row[4],experiment_row[6]))
			elif experiment_row[6] is None and experiment_row[5] is not None:
				final_cursor.execute('''SELECT id FROM experiment where algorithm_id = (?) and workload_id=(?) and column_size=(?) and query_selectivity=(?) and fixed_delta=(?) ''', (experiment_row[1],experiment_row[2],experiment_row[3],experiment_row[4],experiment_row[5]))
			else:
				final_cursor.execute('''SELECT id FROM experiment where algorithm_id = (?) and workload_id=(?) and column_size=(?) and query_selectivity=(?) ''', (experiment_row[1],experiment_row[2],experiment_row[3],experiment_row[4]))
			experiment_exists = final_cursor.fetchone()
			if experiment_exists is None:
				final_cursor.execute('''INSERT INTO experiment(algorithm_id, workload_id, column_size, query_selectivity,fixed_delta,fixed_interactivity_threshold)
					VALUES(:algorithm_id,:workload_id, :column_size, :query_selectivity,:fixed_delta,:fixed_interactivity_threshold)''',
					{'algorithm_id':experiment_row[1], 'workload_id':experiment_row[2], 'column_size':experiment_row[3], 'query_selectivity':experiment_row[4], 'fixed_delta':experiment_row[5], 'fixed_interactivity_threshold':experiment_row[6]})
				final_cursor.execute('''SELECT max(id) FROM experiment ''')
				current_experiment_id = final_cursor.fetchone()
				current_cursor.execute('''SELECT * FROM queries where experiment_id = (?)''', (str(experiment_row[0]),))
				result_queries = current_cursor.fetchall()
				for query_row in result_queries:
					final_cursor.execute('''INSERT INTO queries(experiment_id, query_number, delta, query_time,indexing_time,total_time,pref_sum_total_time)
						VALUES(:experiment_id,:query_number, :delta, :query_time, :indexing_time, :total_time, :pref_sum_total_time)''',
						{'experiment_id':current_experiment_id[0], 'query_number':query_row[1], 'delta':query_row[2], 'query_time':query_row[3], 'indexing_time':query_row[4], 'total_time':query_row[5], 'pref_sum_total_time': query_row[6]})
				final_db.commit()
		current_db.close()
	final_db.close()

download_databases()
create_new_db()
merge_databases()