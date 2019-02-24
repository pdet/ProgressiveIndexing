import sqlite3
db = sqlite3.connect('results.db')
cursor = db.cursor()
cursor.execute('''
    CREATE TABLE algorithm(id INTEGER PRIMARY KEY, name TEXT NOT NULL)
''')
cursor.execute('''
    CREATE TABLE workload(id INTEGER PRIMARY KEY, name TEXT NOT NULL)
''')

cursor.execute('''
    CREATE TABLE experiment(id INTEGER PRIMARY KEY, algorithm_id INTEGER NOT NULL, workload_id INTEGER NOT NULL, column_size INTEGER NOT NULL, query_selectivity INTEGER NOT NULL, fixed_delta REAL, fixed_interactivity_threshold REAL
    , FOREIGN KEY (workload_id) REFERENCES workload(id), FOREIGN KEY (algorithm_id) REFERENCES algorithm(id))
''')
cursor.execute('''
    CREATE TABLE queries(experiment_id INTEGER , query_number INTEGER NOT NULL, delta REAL , query_time REAL NOT NULL,indexing_time REAL NOT NULL,total_time REAL NOT NULL, FOREIGN KEY (experiment_id) REFERENCES experiment(id),  PRIMARY KEY(experiment_id,query_number))
''')

algorithms = [('FullScan',),('FullIndex',),('StandardCracking',),('StochasticCracking',),('ProgressiveStochasticCracking',),('CoarseGranularIndex',), ('ProgressiveQuicksort',),('ProgressiveQuicksortCostModel',),('ProgressiveBucketsortEquiheight',),('ProgressiveBucketsortEquiheightCostModel',) ,('ProgressiveRadixsortLSD',),('ProgressiveRadixsortLSDCostModel',),('ProgressiveRadixsortMSD',),('ProgressiveRadixsortMSDCostModel',)]
cursor.executemany(''' INSERT INTO algorithm(name) VALUES(?)''', algorithms)

workload = [('SkyServer',),('Random',),('SeqOver',),('SeqRand',),('ZoomIn',),('SeqZoomIn',), ('Skew',),('ZoomOutAlt',),('Periodic',),('ZoomInAlt',)]
cursor.executemany(''' INSERT INTO workload(name) VALUES(?)''', workload)

db.commit()
db.close()