sqlite3 -header -csv results_fixed_budget.db " select total_time,cost_model_time from experiments inner join queries on (queries.experiment_id = experiments.id) where algorithm_id = 8 and query_number < 1000;"  > fixed_quick.csv

sqlite3 -header -csv results_fixed_budget.db " select total_time,cost_model_time from experiments inner join queries on (queries.experiment_id = experiments.id) where algorithm_id = 10 and query_number < 1000;"  > fixed_bucket.csv

sqlite3 -header -csv results_fixed_budget.db " select total_time,cost_model_time from experiments inner join queries on (queries.experiment_id = experiments.id) where algorithm_id = 12 and query_number < 1000;"  > fixed_radix_lsd.csv

sqlite3 -header -csv results_msd_fix.db " select total_time,cost_model_time from experiments inner join queries on (queries.experiment_id = experiments.id) where algorithm_id = 14 and query_number < 1000;"  > fixed_radix_msd.csv

sqlite3 -header -csv results_adaptive_budget.db " select total_time,cost_model_time from experiments inner join queries on (queries.experiment_id = experiments.id) where algorithm_id = 8 and query_number < 1000;"  > adaptive_quick.csv

sqlite3 -header -csv results_adaptive_budget.db " select total_time,cost_model_time from experiments inner join queries on (queries.experiment_id = experiments.id) where algorithm_id = 10 and query_number < 1000;"  > adaptive_bucket.csv

sqlite3 -header -csv results_adaptive_budget.db " select total_time,cost_model_time from experiments inner join queries on (queries.experiment_id = experiments.id) where algorithm_id = 12 and query_number < 1000;"  > adaptive_radix_lsd.csv

# sqlite3 -header -csv results_adaptive_budget.db " select total_time,cost_model_time from experiments inner join queries on (queries.experiment_id = experiments.id) where algorithm_id = 14 and query_number < 1000;"  > adaptive_radix_msd.csv

sqlite3 -header -csv  results_msd_adp.db " select total_time,cost_model_time from experiments inner join queries on (queries.experiment_id = experiments.id) where algorithm_id = 14 and query_number < 1000;"  > adaptive_radix_msd.csv
