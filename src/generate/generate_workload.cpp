#include <stdio.h>
#include <string>
#include <assert.h>
#include <iostream>

#include <algorithm>
#include "../include/generate/random.h"
#include "../include/util/file_manager.h"
#include "../include/structs.h"
#include "../include/full_index/hybrid_radix_insert_sort.h"
#include "../include/full_index/bulkloading_bp_tree.h"

using namespace std;

string COLUMN_FILE_PATH, QUERIES_FILE_PATH, ANSWER_FILE_PATH;
float SELECTIVITY_PERCENTAGE;
int NUM_QUERIES, COLUMN_SIZE, QUERIES_PATTERN;

class Workload {
    int N;    // the number of elements in arr
    int W;    // the selected workload to be generated
    int S;    // the selectivity (unused for some workloads)
    int I = 0;    // the I'th query (internal use only)
    int64_t a, b; // the last query range [a,b]
    Random r; // Pseudo Random Generator

    // based on the predefined queries from file
    bool skyserver_w() {
        static FILE *in = NULL;
        if (I == 0) {
            in = fopen("real_data/skyserver/skyserver.queries", "r");
            if (!in) fprintf(stderr, "Fail loading file real_data/skyserver/skyserver.queries\n");
        }
        if (!in) return false;
        double x, y;
        if (fscanf(in, "%lf %lf", &x, &y) == EOF) {
            if (in) {
                fclose(in);
                in = NULL;
            }
            return false;
        }
        a = int(y * 1000000);
        b = a + S;
        return true;
    }

    // a and b is selected uniformly at random and a < b
    bool random_w() {
        a = rand() % (N - S);
        b = a + S;
        return true;
    }

    // a will be incremented by 10 every subsequent query
    // the range may overlap with the next queries
    bool seq_over_w() {
        int j = N / NUM_QUERIES;
        a = I * j;
        b = a + S;
        return b <= N;
    }


    // half the time is seq_over half the time is random_w
    bool seq_rand_w() {
        int j = N / NUM_QUERIES;
        a = I * j;
        b = a + rand() % (N - I * j);
        return true;
    }


    // start at the [middle - 100500, middle + 100500),
    // then zoom in by making the query range smaller by 100 on each query
    bool zoom_in_w() {
        static int L;
        if (!I) L = N / 3;
        static int R;
        if (!I) R = 2 * N / 3;
        if (L >= R || L < 0 || R > N) return false;
        a = L;
        L += N / 100000;  // make the range smaller
        b = R;
        R -= N / 100000;
        return true;
    }


    // after zooming in on one region, move to next unexplored region to the right
    bool seq_zoom_in() {
        static int L;
        if (!I) L = 1;
        static int G = 100000;
        static int R;
        if (!I) R = G;
        if (L >= R) L += G, R = L + G;
        if (R > N) return false;
        a = L;
        L += 100;
        b = R;
        R -= 100;
        return true;
    }


    //where 80 percent of the queries falls within 20 percent of the value range and
    //20 percent of the queries falls within the 80 percent of the value range
    bool skew_w() {
        if (I >= 10000) return false;
        if (I < 8000) {
            do {
                a = r.nextInt(N / 5);
                b = r.nextInt(N / 5);
            } while (a == b);
        } else {
            do {
                a = N / 5 + r.nextInt((N * 4) / 5);
                b = N / 5 + r.nextInt((N * 4) / 5);
            } while (a == b);
        }
        b = a + S;
        // if (a > b) swap(a, b);
        return true;
    }

    // start at the [middle - 500, middle + 500),
    // then zoom out by making the query range larger by 100 each query
    bool zoom_out_alt_w() {
        static int L;
        if (!I) L = N / 2 - 500;
        static int R;
        if (!I) R = N / 2 + 500;
        if (L < 1 || R > N) return false;
        if (I & 1) {
            a = L;
            L -= 100;  // make the range bigger
            b = a + 10;
        } else {
            b = R;
            R += 100;
            a = b - 10;
        }
        b = a + S;
        return true;
    }

    bool periodic_w() {
        static long long jump = N / 100;
        a = (I * jump) % (N - S);
        b = a + S;
        return true;
    }

    bool zoom_in_alt_w() {
        int x = pow(-1, I);
        int j = N / NUM_QUERIES;
        a = x * I * j + (N - S) * (1 - x) / 2;
        b = a + S;
        return true;
    }

public :
    Workload(int nElem, int workload, int selectivity) : N(nElem), W(workload), S(selectivity) {
        r = Random(29284);
    }

    bool query(int64_t &na, int64_t &nb) {
        switch (W) {
            case 1 :
                if (!skyserver_w()) return false;
                break;
            case 2 :
                if (!random_w()) return false;
                break;
            case 3 :
                if (!seq_over_w()) return false;
                break;
            case 4 :
                if (!seq_rand_w()) return false;
                break;
            case 5 :
                if (!zoom_in_w()) return false;
                break;
            case 6 :
                if (!seq_zoom_in()) return false;
                break;
            case 7 :
                if (!skew_w()) return false;
                break;
            case 8 :
                if (!zoom_out_alt_w()) return false;
                break;
            case 9 :
                if (!periodic_w()) return false;
                break;
            case 10 :
                if (!zoom_in_alt_w()) return false;
                break;
            default :
                assert(0);
        }
        na = a;
        nb = b;
        I++;
        return true;
    }
};

void print_help(int argc, char **argv) {
    fprintf(stderr, "Unrecognized command line option.\n");
    fprintf(stderr, "Usage: %s [args]\n", argv[0]);
    fprintf(stderr, "   --column-path\n");
    fprintf(stderr, "   --query-path\n");
    fprintf(stderr, "   --answer-path\n");
    fprintf(stderr, "   --selectiviy\n");
    fprintf(stderr, "   --num-queries\n");
    fprintf(stderr, "   --queries-pattern\n");
    fprintf(stderr, "   --print-mode\n");

}

pair<string, string> split_once(string delimited, char delimiter) {
    auto pos = delimited.find_first_of(delimiter);
    return {delimited.substr(0, pos), delimited.substr(pos + 1)};
}

void *fullIndex(IndexEntry *c) {
    hybrid_radixsort_insert(c, COLUMN_SIZE);
    void *I = build_bptree_bulk(c, COLUMN_SIZE);
    return I;
}

int64_t scanQuery(IndexEntry *c, int64_t from, int64_t to) {
    int64_t sum = 0;
    for (int64_t i = from; i <= to; i++) {
        sum += c[i].m_key;
    }

    return sum;
}

int main(int argc, char **argv) {
    COLUMN_FILE_PATH = "column";
    QUERIES_FILE_PATH = "query";
    ANSWER_FILE_PATH = "answer";
    SELECTIVITY_PERCENTAGE = 1;
    NUM_QUERIES = 20;
    COLUMN_SIZE = 100000000;
    QUERIES_PATTERN = 2;
    vector<int64_t> leftQuery;
    vector<int64_t> rightQuery;
    vector<int64_t> queryAnswer;

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
        } else if (arg_name == "query-path") {
            QUERIES_FILE_PATH = arg_value;
        } else if (arg_name == "answer-path") {
            ANSWER_FILE_PATH = arg_value;
        } else if (arg_name == "selectivity") {
            SELECTIVITY_PERCENTAGE = atof(arg_value.c_str());
        } else if (arg_name == "num-queries") {
            NUM_QUERIES = atoi(arg_value.c_str());
        } else if (arg_name == "column-size") {
            COLUMN_SIZE = atoi(arg_value.c_str());
        } else if (arg_name == "queries-pattern") {
            QUERIES_PATTERN = atoi(arg_value.c_str());
        } else {
            print_help(argc, argv);
            exit(EXIT_FAILURE);
        }
    }

    Workload W(COLUMN_SIZE, QUERIES_PATTERN, COLUMN_SIZE / 100 * SELECTIVITY_PERCENTAGE);
    int64_t a, b;
    Column c;
    IndexEntry *data;
    BulkBPTree *T;
    c.data = vector<int64_t>(COLUMN_SIZE);
    load_column(&c, COLUMN_FILE_PATH, COLUMN_SIZE);
    data = (IndexEntry *) malloc(COLUMN_SIZE * 2 * sizeof(int64_t));
    for (size_t i = 0; i < COLUMN_SIZE; i++) {
        data[i].m_key = c.data[i];
        data[i].m_rowId = i;
    }
    T = (BulkBPTree *) fullIndex(data);

    for (size_t i = 0; i < NUM_QUERIES; i++) {
        W.query(a, b);
        leftQuery.push_back(a);
        // Weird binary search edge-case
        if (b == COLUMN_SIZE)
            b-=2;
        rightQuery.push_back(b);

        // Using Full Index to generate answers to all workloads.
        int64_t offset1 = (T)->gte(a);
        int64_t offset2 = (T)->lte(b);
        int64_t sum = scanQuery(data, offset1, offset2);
        queryAnswer.push_back(sum);

    }
    FILE *f = fopen(QUERIES_FILE_PATH.c_str(), "w+");
    fwrite(&leftQuery[0], sizeof(int64_t), NUM_QUERIES, f);
    fwrite(&rightQuery[0], sizeof(int64_t), NUM_QUERIES, f);
    fclose(f);
    FILE *f_2 = fopen(ANSWER_FILE_PATH.c_str(), "w+");
    fwrite(&queryAnswer[0], sizeof(int64_t), NUM_QUERIES, f_2);
    fclose(f_2);


}
