#include <stdio.h>
#include <string>
#include <assert.h>
#include <iostream>

#include <algorithm>
#include "../include/generate/random.h"
#include "../include/util/file_manager.h"
#include "../include/structs.h"


using namespace std;


class Workload {
    int N;    // the number of elements in arr
    int W;    // the selected workload to be generated
    int S;    // the selectivity (unused for some workloads)
    int I = 0;    // the I'th query (internal use only)
    int a, b; // the last query range [a,b]
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
        if (S == 0) { // random selectivity
            do {
                a = r.nextInt(N);
                b = r.nextInt(N);
                if (a > b) swap(a, b);
            } while (a == b);
        } else {
            a = r.nextInt(N - S + 1);
            b = max(a + 1, a + S);
        }
        return true;
    }

    // a will be incremented by 10 every subsequent query
    // the range may overlap with the next queries
    bool seq_over_w() {
        a = 10 + I * 20;
        if (a + 5 > N) return false;
        if (S == 0) {
            b = a + r.nextInt(N - a) + 1;
        } else {
            b = a + S;
        }
        return b <= N;
    }

    // the opposit direction from the seq_over_w
    bool seq_inv_w() {
        if (!seq_over_w()) return false;
        a = N - a;
        b = N - b;
        swap(a, b);
        return true;
    }

    // half the time is seq_over half the time is random_w
    bool seq_rand_w() {
        if (I & 1) {
            return seq_over_w();
        } else {
            return random_w();
        }
    }

    // sequential with no overlap with the subsequent query ranges
    bool seq_no_over_w() {
        static int prevB;
        if (!I) prevB = 0;
        a = prevB + 10;
        if (a + 5 > N) return false;
        if (S == 0) {
            b = a + r.nextInt(N - a) + 1;
        } else {
            b = a + S;
        }
        prevB = b;
        return b <= N;
    }

    // sequential alternate at the beginning and end
    bool seq_alt_w() {
        if (I & 1) {
            return seq_over_w();
        } else {
            return seq_inv_w();
        }
    }

    // pick 1000 integers and produce range queries with endpoints
    // using the 1000 picked integers
    bool cons_rand_w() {
        static int R[1000];
        if (!I) for (int i = 0; i < 1000; i++) R[i] = r.nextInt(N);
        do {
            a = R[r.nextInt(1000)];
            b = R[r.nextInt(1000)];
        } while (a == b);
        if (a > b) swap(a, b);
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
        L += 100;  // make the range smaller
        b = R;
        R -= 100;
        return true;
    }

    // start at the [middle - 500, middle + 500),
    // then zoom out by making the query range larger by 100 each query
    bool zoom_out_w() {
        static int L;
        if (!I) L = N / 2 - 500;
        static int R;
        if (!I) R = N / 2 + 500;
        if (L < 1 || R > N) return false;
        a = L;
        L -= 100;  // make the range bigger
        b = R;
        R += 100;
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

    // after zooming out on one ragion, move to the next unexplored region on the right
    bool seq_zoom_out() {
        static int G = 100000;
        static int L;
        if (!I) L = G / 2 + 1000;
        static int R;
        if (!I) R = L + 10;
        if (R > L + G) L = R + G / 2 + 1000, R = L + 10;
        if (R > N) return false;
        a = L;
        L -= 100;
        b = R;
        R += 100;
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
        if (a > b) swap(a, b);
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
        return true;
    }

    // start at the [middle - 500, middle + 500),
    // then zoom out by making the query range larger by 100 each query
    bool skew_zoom_out_alt_w() {
        static int L;
        if (!I) L = N - 355000;
        static int R;
        if (!I) R = N - 350000;
        if (L < 1 || R > N) return false;
        if (I & 1) {
            b = R;
            R += 20;
            a = b - 10;
        } else {
            a = L;
            L -= 20;  // make the range bigger
            b = a + 10;
        }
        return true;
    }

    bool periodic_w() {
        static long long jump = 1000001;
        a = (I * jump) % N;
        b = a + 10;
        return true;
    }

    bool mixed_w() {
        static int work = 0;
        static int base = 0;
        if (I % 1000 == 0) {
            work = r.nextInt(15) + 1;
            base = r.nextInt(20);
        }
        int tW = W;
        W = work;
        int tI = I;
        I %= 1000;
        int tN = N;
        N /= 20;
        int64_t ta, tb;
        bool ok = query(ta, tb);
        W = tW;
        I = tI;
        if (!ok) {
            N = tN;
            work = r.nextInt(15) + 1;
            return mixed_w();
        }
        a = ta + base * N;
        b = tb + base * N;
        N = tN;
        return true;
    }

public :
    Workload(int nElem, int workload, int selectivity) : N(nElem), W(workload), S(selectivity) {
        r = Random(29284);
    }

    bool query(int64_t &na, int64_t &nb) {
        switch (W) {
            case 0 :
                if (!skyserver_w()) return false;
                break;
            case 1 :
                if (!random_w()) return false;
                break;
            case 2 :
                if (!seq_over_w()) return false;
                break;
            case 3 :
                if (!seq_inv_w()) return false;
                break;
            case 4 :
                if (!seq_rand_w()) return false;
                break;
            case 5 :
                if (!seq_no_over_w()) return false;
                break;
            case 6 :
                if (!seq_alt_w()) return false;
                break;
            case 7 :
                if (!cons_rand_w()) return false;
                break;
            case 8 :
                if (!zoom_in_w()) return false;
                break;
            case 9 :
                if (!zoom_out_w()) return false;
                break;
            case 10 :
                if (!seq_zoom_in()) return false;
                break;
            case 11 :
                if (!seq_zoom_out()) return false;
                break;
            case 12 :
                if (!skew_w()) return false;
                break;
            case 13 :
                if (!zoom_out_alt_w()) return false;
                break;
            case 14 :
                if (!skew_zoom_out_alt_w()) return false;
                break;
            case 15 :
                if (!periodic_w()) return false;
                break;
            case 16 :
                if (!mixed_w()) return false;
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

int64_t check_answer(int64_t low, int64_t high){
    int64_t answer;
    size_t size;
    if (low == 0)
        size = high - low;
    else
        size = high - low + 1;
    if (size % 2 == 0)
        answer = (high + low) * size/2;
    else
        answer = (high + low) * (size/2) + ((high + low)/2);
    return answer;
}

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

int main(int argc, char **argv) {
    string COLUMN_FILE_PATH, QUERIES_FILE_PATH, ANSWER_FILE_PATH;
    float SELECTIVITY_PERCENTAGE;
    int NUM_QUERIES, COLUMN_SIZE, QUERIES_PATTERN;

    COLUMN_FILE_PATH = "column";
    QUERIES_FILE_PATH = "query";
    ANSWER_FILE_PATH = "answer";
    SELECTIVITY_PERCENTAGE = 0.01;
    NUM_QUERIES = 150;
    COLUMN_SIZE = 100000000;
    QUERIES_PATTERN = 1;
    vector<int64_t> leftQuery;
    vector<int64_t> rightQuery;
    vector<int64_t> queryAnswer;
    bool print_mode = 0;
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
        } else if (arg_name == "print-mode") {
            print_mode = atoi(arg_value.c_str());
        } else {
            print_help(argc, argv);
            exit(EXIT_FAILURE);
        }
    }
    if(print_mode){
        Column c;
        c.data = vector<int64_t>(COLUMN_SIZE);
        load_column(&c, COLUMN_FILE_PATH, COLUMN_SIZE);
        Workload W(COLUMN_SIZE, QUERIES_PATTERN, COLUMN_SIZE/100 * SELECTIVITY_PERCENTAGE);
        int64_t a, b;
        for (size_t i = 0; i < NUM_QUERIES; i++) {
            W.query(a, b);
            leftQuery.push_back(a);
            rightQuery.push_back(b);
            cout << i+1 << ";" << a << ";"<< b <<"\n";
        }
    }
    else{
        if (!file_exists(QUERIES_FILE_PATH)) {
            Column c;
            c.data = vector<int64_t>(COLUMN_SIZE);
            load_column(&c, COLUMN_FILE_PATH, COLUMN_SIZE);
            Workload W(COLUMN_SIZE, QUERIES_PATTERN, COLUMN_SIZE/100 * SELECTIVITY_PERCENTAGE);
            int64_t a, b;
            for (size_t i = 0; i < NUM_QUERIES; i++) {
                W.query(a, b);
                leftQuery.push_back(a);
                rightQuery.push_back(b);
                // The skyserver workload doesn'' follow the same column distribution as other queries
                // Hence a scan is performed to generate the query answers
                if (QUERIES_PATTERN == 1){
                    int64_t sum = 0;
                    for (size_t j = 0; j < COLUMN_SIZE; j++)
                        if (c.data[j] >= a && c.data[j] <= b)
                            sum += c.data[j];
                    queryAnswer.push_back(sum);
                }
                else{
                    queryAnswer.push_back(check_answer(a,b));
                }
            }
            FILE *f = fopen(QUERIES_FILE_PATH.c_str(), "w+");
            fwrite(&leftQuery[0], sizeof(int64_t), NUM_QUERIES, f);
            fwrite(&rightQuery[0], sizeof(int64_t), NUM_QUERIES, f);
            fclose(f);
            FILE *f_2 = fopen(ANSWER_FILE_PATH.c_str(), "w+");
            fwrite(&queryAnswer[0], sizeof(int64_t), NUM_QUERIES, f_2);
            fclose(f_2);

        } else {
            fprintf(stderr, "File already exists, delete it first if you want to generate it again.\n");
        }
    }

}
