import os
import sys
import inspect

SCRIPT_PATH =  os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe()))) # script directory

def combine_results(directory, base,output):
	global first_file
	current_directory = os.path.join(base, directory)
	print current_directory
	files = os.listdir(current_directory)
	for f in files:
		fullpath = os.path.join(current_directory, f)
		if os.path.isdir(fullpath):
			combine_results(f, current_directory,output)
		elif f == 'results.csv':
			with open(fullpath, 'r') as new_file:
				for line in new_file:
					if line[-1] != '\n':
						continue
					if line.count(';') == 10:
						splits = line.split(';')
						splits.insert(len(splits)-1, '')
						line = ';'.join(splits)
					if line.count(';') != 11:
						continue
					try:
						float(line.split(';')[-1])
						output.write(line)
					except:
						pass
			output.flush()

def run(directory,base):
	output = open('output'+base+'.csv', 'w+')
	output.write("algorithm;repetition;query_selectivity;query_number;query_pattern;point_query;column_size;column_pattern;allowed_swap;delta;converged;query_time\n")

	if len(sys.argv) > 1:
		os.chdir(sys.argv[1])
	combine_results(directory,base,output)

def plot(experiment_directory,experiment_base,final_folder):
	run(experiment_directory,experiment_base)
	os.system("mv output"+experiment_base+".csv "+final_folder+"/output.csv")
	os.system("cp plot.r "+final_folder+"/")
	os.chdir(os.path.join(SCRIPT_PATH,final_folder))
	os.system('Rscript plot.r')
	os.system('rm plot.r')
	os.chdir(os.path.join(SCRIPT_PATH))

def deltaplot(experiment_directory,experiment_base,final_folder):
	run(experiment_directory,experiment_base)
	os.system("mv output"+experiment_base+".csv "+final_folder+"/output.csv")
	os.chdir(os.path.join(SCRIPT_PATH,final_folder))
	os.system('Rscript plot.r')
	os.chdir(os.path.join(SCRIPT_PATH))

BASE_PATH ='/Users/holanda/ResultsCSV/'
print("test")
# if os.path.exists(BASE_PATH+"1_1_0"):
# 	plot(BASE_PATH+'1_1_0','1_1_0',"RandomQueries")
# if os.path.exists(BASE_PATH+"1_1_1"):
# 	plot(BASE_PATH+'1_1_1','1_1_1',"PointQueries")
# if os.path.exists(BASE_PATH+"1_2_0"):
# 	plot(BASE_PATH+'1_2_0','1_2_0',"SequentialQueries")
# if os.path.exists(BASE_PATH+"1_3_0"):
# 	plot(BASE_PATH+'1_3_0','1_3_0',"SkewedQueries")
# if os.path.exists(BASE_PATH+"1_4_0"):
# 	plot(BASE_PATH+'1_4_0','1_4_0',"MixedQueries")
# if os.path.exists(BASE_PATH+"3_3_0"):
# 	plot(BASE_PATH+'3_3_0','3_3_0',"SkewedColumn")
# if os.path.exists(BASE_PATH+"Deltas"):
# 	deltaplot(BASE_PATH+'Deltas','Deltas',"DeltaExperiment")