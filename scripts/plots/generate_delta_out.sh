sqlite3 -header -csv results_fixeddeltas.db " select algorithm_id,fixed_delta,query_number,total_time from experiments inner join queries on (queries.experiment_id = experiments.id) where fixed_delta is not null and query_number == 0;"  > out_delta_first_query.csv

sqlite3 -header -csv results_fixeddeltas.db " select algorithm_id,fixed_delta,sum(total_time) as sum_time from experiments inner join queries on (queries.experiment_id = experiments.id) where fixed_delta is not null group by algorithm_id,fixed_delta;"  > out_delta_sum.csv

sqlite3 -header -csv results_fixeddeltas.db " select algorithm_id,fixed_delta,min(query_number) as convergence_nr from experiments inner join queries on (queries.experiment_id = experiments.id) where fixed_delta is not null and total_time < 0.001 group by algorithm_id,fixed_delta;"  > out_delta_conv.csv

sqlite3 -header -csv results_fixeddeltas.db " select algorithm_id,fixed_delta,min(query_number) as convergence_nr from experiments inner join queries on (queries.experiment_id = experiments.id) where fixed_delta is not null and pref_sum_total_time < (query_number+1)*0.70 group by algorithm_id,fixed_delta;"  > out_delta_pay_off.csv
