import os
import sys

BASE_PATH =os.getcwd()

COLUMN_SIZE_LIST = [100000000,1000000000]
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
					if line.count(';') == 3:
						splits = line.split(';')
						splits.insert(len(splits)-1, '')
						line = ';'.join(splits)
					if line.count(';') != 4:
						continue
					try:
						float(line.split(';')[-1])
						output.write(line)
					except:
						pass
			output.flush()

def run(directory,base):
	output = open(base+'.csv', 'w+')
	output.write("algorithm;query_number;query_pattern;delta;query_time\n")
	combine_results(directory,base,output)

def plot_queries(experiment_directory,experiment_base):
	final_folder = "plots/"+experiment_base
	os.system("mkdir -p "+ final_folder)
	run(experiment_directory,experiment_base)
	os.system("mv "+experiment_base+".csv "+final_folder+"/output.csv")
	os.system("cp scripts/plots/plot_queries.R "+final_folder+"/")
	os.chdir(os.path.join(BASE_PATH,final_folder))
	os.system('Rscript plot_queries.R')
	os.system('rm plot_queries.R')
	os.chdir(os.path.join(BASE_PATH))

def deltaplot(experiment_directory,experiment_base,final_folder):
	run(experiment_directory,experiment_base)
	os.system("mv output"+experiment_base+".csv "+final_folder+"/output.csv")
	os.chdir(os.path.join(SCRIPT_PATH,final_folder))
	os.system('Rscript plot.r')
	os.chdir(os.path.join(SCRIPT_PATH))

os.system("rm -r " + BASE_PATH + "/plots/")
for column_size in COLUMN_SIZE_LIST:
	for workload in range(0,17):
		if os.path.exists(BASE_PATH+ "/ResultsCSV/"+str(column_size)+"_"+str(workload)):		
			plot_queries(BASE_PATH+ "/ResultsCSV/"+str(column_size)+"_"+str(workload),str(column_size)+"_"+str(workload))

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