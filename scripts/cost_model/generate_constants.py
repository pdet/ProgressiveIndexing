import os
import inspect

os.system('g++ -std=c++11 -O3 scripts/cost_model/measure.cpp -march=native')
os.system('rm src/include/progressive/constants.h')
result = os.popen("./a.out").read()
os.system('rm ./a.out')
file = open('src/include/progressive/constants.h',"w")

file.write('#ifndef PROGRESSIVEINDEXING_CONSTANTS_H\n')
file.write('#define PROGRESSIVEINDEXING_CONSTANTS_H\n\n')
file.write('#define PAGESIZE 4096.0\n')
file.write('#define ELEMENTS_PER_PAGE (PAGESIZE / sizeof(int64_t))\n\n')

file.write(result + '\n')

file.write('\n#endif //PROGRESSIVEINDEXING_CONSTANTS_H')

file.close()