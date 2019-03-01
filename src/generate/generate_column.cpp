#include <stdio.h>
#include <assert.h>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include "../include/generate/random.h"
#include "../include/util/file_manager.h"

using namespace std;
int COLUMN_SIZE;
string COLUMN_FILE_PATH;

// int range = max - min + 1;
// int num = rand() % range + min;

int generate_column(int N) {
    vector<int> data;
    for (int i = 0; i < N ; i ++){
        int aux,min,max;
        int variance = rand()%10;
        if (variance == 0){
            min = 0;
            max = N*0.49;
            aux = rand()%(max-min + 1) + min; // 0 - 40
        }
        else if (variance == 9){
            min = N*0.5;
            max = N;
            aux = rand()%(max-min + 1) + min;
        }
        else {//40 -50
            min = N*0.49 ;
            max = N*0.5;
            aux = rand()%(max-min + 1) + min;
        }
        data.push_back(aux);
    }
    random_shuffle(data.begin(), data.end());
    FILE *f = fopen(COLUMN_FILE_PATH.c_str(), "w+");
    fwrite(&data[0], sizeof(int), N, f);
    fclose(f);
}




void print_help(int argc, char **argv) {
    fprintf(stderr, "Unrecognized command line option.\n");
    fprintf(stderr, "Usage: %s [args]\n", argv[0]);
    fprintf(stderr, "   --column-path\n");
    fprintf(stderr, "   --column-size\n");
}

pair<string, string> split_once(string delimited, char delimiter) {
    auto pos = delimited.find_first_of(delimiter);
    return {delimited.substr(0, pos), delimited.substr(pos + 1)};
}

int main(int argc, char **argv) {
    COLUMN_FILE_PATH = "column";
    COLUMN_SIZE = 100;

    for (int i = 1; i < argc; i++) {
        auto arg = string(argv[i]);
        if (arg.substr(0, 2) != "--") {
            print_help(argc, argv);
            exit(EXIT_FAILURE);
        }
        arg = arg.substr(2);
        auto p = split_once(arg, '=');
        auto &arg_name = p.first;
        auto &arg_value = p.second;
        if (arg_name == "column-path") {
            COLUMN_FILE_PATH = arg_value;
        } else if (arg_name == "column-size") {
            COLUMN_SIZE = atoi(arg_value.c_str());
        } else {
            print_help(argc, argv);
            exit(EXIT_FAILURE);
        }
    }
//    if (!file_exists(COLUMN_FILE_PATH)) {
        generate_column(COLUMN_SIZE);
//    } else {
//        fprintf(stderr, "File already exists, delete it first if you want to generate it again.\n");
//    }

}