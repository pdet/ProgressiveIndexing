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
    CREATE TABLE experiment_baseline(id INTEGER PRIMARY KEY, algorithm_id INTEGER NOT NULL, workload_id INTEGER NOT NULL, column_size INTEGER NOT NULL, query_selectivity INTEGER NOT NULL
    , FOREIGN KEY (workload_id) REFERENCES workload(id), FOREIGN KEY (algorithm_id) REFERENCES algorithm(id))
''')
cursor.execute('''
    CREATE TABLE times_baseline(experiment_id INTEGER , query_number INTEGER NOT NULL, time_seconds REAL NOT NULL, FOREIGN KEY (experiment_id) REFERENCES experiment(id))
''')
cursor.execute('''
    CREATE TABLE experiment_progressive(id INTEGER PRIMARY KEY, algorithm_id INTEGER NOT NULL, workload_id INTEGER NOT NULL, column_size INTEGER NOT NULL, query_selectivity INTEGER NOT NULL, fixed_delta REAL NOT NULL
    , FOREIGN KEY (workload_id) REFERENCES workload(id), FOREIGN KEY (algorithm_id) REFERENCES algorithm(id))
''')
cursor.execute('''
    CREATE TABLE times_progressive(experiment_id INTEGER , query_number INTEGER NOT NULL, time_seconds REAL NOT NULL, FOREIGN KEY (experiment_id) REFERENCES experiment(id))
''')
cursor.execute('''
    CREATE TABLE experiment_progressive_cm(id INTEGER PRIMARY KEY, algorithm_id INTEGER NOT NULL, workload_id INTEGER NOT NULL, column_size INTEGER NOT NULL, query_selectivity INTEGER NOT NULL, fixed_interactivity_threshold REAL
    , FOREIGN KEY (workload_id) REFERENCES workload(id), FOREIGN KEY (algorithm_id) REFERENCES algorithm(id))
''')
cursor.execute('''
    CREATE TABLE times_progressive_cm(experiment_id INTEGER , query_number INTEGER NOT NULL, delta REAL not null, time_seconds REAL NOT NULL, FOREIGN KEY (experiment_id) REFERENCES experiment(id))
''')

algorithms = [('FullScan',),('FullIndex',),('StandardCracking',),('StochasticCracking',),('ProgressiveStochasticCracking',),('CoarseGranularIndex',), ('ProgressiveQuicksort',),('ProgressiveQuicksortCostModel',),('ProgressiveBucketsortEquiheight',),('ProgressiveBucketsortEquiheightCostModel',) ,('ProgressiveRadixsortLSD',),('ProgressiveRadixsortLSDCostModel',),('ProgressiveRadixsortMSD',),('ProgressiveRadixsortMSDCostModel',)]
cursor.executemany(''' INSERT INTO algorithm(name) VALUES(?)''', algorithms)
workload = [('SkyServer',),('Random',),('SeqOver',),('SeqInv',),('SeqRand',),('SeqNoOver',), ('SeqAlt',),('ConsRandom',),('ZoomIn',),('ZoomOut',) ,('SeqZoomIn',),('SeqZoomOut',),('Skew',)
,('ZoomOutAlt',),('SkewZoomOutAlt',),('Periodic',),('Mixed',)]
cursor.executemany(''' INSERT INTO workload(name) VALUES(?)''', workload)

db.commit()
db.close()