import os

def generate_tpch(sf=1):
	os.system('rm -rf tpch-dbgen')
	os.system('git clone https://github.com/eyalroz/tpch-dbgen')
	os.chdir('tpch-dbgen')
	os.system('cmake -G "Unix Makefiles" .')
	os.system('make')
	os.system('./dbgen -s ${SCALE_FACTOR}'.replace("${SCALE_FACTOR}", str(sf)))
	# remove trailing |
	for f in os.listdir('.'):
		if len(f) < 4 or f[-4:] != '.tbl': continue
		os.system('rm -f ${FILE}.tmp'.replace("${FILE}", f))
		os.system("sed 's/.$//' ${FILE} > ${FILE}.tmp".replace("${FILE}", f))
		os.system('mv ${FILE}.tmp ${FILE}'.replace("${FILE}", f))

generate_tpch(0.1)