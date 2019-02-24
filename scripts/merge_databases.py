import os
import inspect
import sqlite3

SCRIPT_PATH =  os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe()))) # script directory
os.chdir(SCRIPT_PATH)
os.chdir("..")

experiments = ["skyserver_baseline", "skyserver_progressive_indexing", "all_fullscan", "skyserver_progressive_indexing_cm"] 
machines = ["stones01","stones03","stones07","stones04" ]

def download_databases():
	for i in range(0,len(experiments)):
		os.system("scp -r "+machines[i]+":/export/scratch1/home/holanda/ProgressiveIndexing/results.db ./"+experiments[i]+".db")

def create_new_db():
	os.system("rm results.db")
	os.system("python scripts/sqlite.py")

def merge_databases():
	final_db = sqlite3.connect('results.db')
	final_cursor = final_db.cursor()
	current_experiment_id = 1
	for i in range(0,len(experiments)):
		current_db = sqlite3.connect(experiments[i]+'.db')
		current_cursor = current_db.cursor()
		current_cursor.execute('''SELECT * FROM experiment''')
		result_experiment= current_cursor.fetchall()
		for experiment_row in result_experiment:
			print(experiment_row)
			final_cursor.execute('''INSERT INTO experiment(algorithm_id, workload_id, column_size, query_selectivity,fixed_delta,fixed_interactivity_threshold)
				VALUES(:algorithm_id,:workload_id, :column_size, :query_selectivity,:fixed_delta,:fixed_interactivity_threshold)''',
				{'algorithm_id':experiment_row[1], 'workload_id':experiment_row[2], 'column_size':experiment_row[3], 'query_selectivity':experiment_row[4], 'fixed_delta':experiment_row[5], 'fixed_interactivity_threshold':experiment_row[6]})
			print(str(experiment_row[0]))
			current_cursor.execute('''SELECT * FROM queries where experiment_id = (?)''', (str(experiment_row[0]),))
			result_queries = current_cursor.fetchall()
			for query_row in result_queries:
				final_cursor.execute('''INSERT INTO queries(experiment_id, query_number, delta, query_time,indexing_time,total_time)
					VALUES(:experiment_id,:query_number, :delta, :query_time, :indexing_time, :total_time)''',
					{'experiment_id':current_experiment_id, 'query_number':query_row[1], 'delta':query_row[2], 'query_time':query_row[3], 'indexing_time':query_row[4], 'total_time':query_row[5]})
			current_experiment_id = current_experiment_id+1
			final_db.commit()
		current_db.close()
	final_db.close()

def generate_csvs():
	SELECT algorithm_id, query_number,delta,query_time,indexing_time,total_time from queries inner join experiment on (experiment.id = queries.experiment_id) where algorithm_id in (1,2,3,4,5,6) and workload_id = 1 and query_selectivity = 0.001;
sqlite3 -header -csv results.db "SELECT algorithm_id, query_number,delta,query_time,indexing_time,total_time from queries inner join experiment on (experiment.id = queries.experiment_id) where algorithm_id in (8,10,12,14) and workload_id = 1 and query_selectivity = 0.001 and fixed_interactivity_threshold = 1.5 and query_number > 0;"  > out.csv
	print("generate")


# download_databases()
create_new_db()
merge_databases()