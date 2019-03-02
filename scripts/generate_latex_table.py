import sqlite3
db = sqlite3.connect('results.db')
cursor = db.cursor()

query = "select work_name,query_selectivity,alg_name, sum(total_time), avg(square) from (select work_name,query_selectivity,alg_name, total_time, ((total_time - scan_time)*(total_time - scan_time)) as square from (select workload.name as work_name, query_selectivity,algorithm.name as alg_name,total_time, (select query_time*1.2 from queries where experiment_id = (select id from experiment where algorithm_id=1 and workload_id = exp.workload_id and query_selectivity = exp.query_selectivity) and query_number=1) as scan_time from experiment as exp inner join algorithm on (algorithm.id = exp.algorithm_id) inner join workload on (workload.id = exp.workload_id) inner join queries on (queries.experiment_id = exp.id) where algorithm_id in (8,10,12,14) and query_selectivity != 10  and total_time > 0.0001 order by workload_id, query_selectivity,algorithm_id)) as tbl group by work_name,query_selectivity,alg_name"

query = "select work_name,query_selectivity,alg_name, sum(total_time)*10, avg(square) from (select work_name,query_selectivity,alg_name, total_time, ((total_time - scan_time)*(total_time - scan_time))*10 as square from (select workload.name as work_name, query_selectivity,algorithm.name as alg_name,total_time, 0.07*1.2 as scan_time from experiment as exp inner join algorithm on (algorithm.id = exp.algorithm_id) inner join workload on (workload.id = exp.workload_id) inner join queries on (queries.experiment_id = exp.id) where query_selectivity != 10  and total_time > 0.000001 order by workload_id, query_selectivity,algorithm_id)) as tbl group by work_name,query_selectivity,alg_name;"

cursor.execute(query)
result= cursor.fetchall()
for row in result:
	print str(row[0]) + "|" +str(row[1]) + "|" +str(row[2]) + "|" +str(row[3]) + "|" + str(row[4]** 0.5)