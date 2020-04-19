#include <chrono>
#include <cmath>
#include <iostream>
#include <jemalloc/jemalloc.h>
#include <vector>

using namespace std;
using namespace chrono;

int main(int argc, char** argv) {
    time_point<system_clock> startTimer, endTimer;

    int size = pow(10, 4);
    int64_t* data;
    //! check malloc
    startTimer = system_clock::now();
    data = (int64_t*)malloc(sizeof(int64_t) * size);
    endTimer = system_clock::now();

    cout << "Allocate with malloc: " << duration<double>(endTimer - startTimer).count() << endl;

    startTimer = system_clock::now();
    for (size_t i = 0; i < size; i++) {
        data[i] = i;
    }
    endTimer = system_clock::now();
    auto s = duration<double>(endTimer - startTimer).count();
    cout << "Fill with malloc: " << duration<double>(endTimer - startTimer).count() << endl;
    free(data);

    //! check jemalloc
    startTimer = system_clock::now();
    data = (int64_t*)malloc(sizeof(int64_t) * size);
    endTimer = system_clock::now();

    cout << "Allocate with jemalloc: " << duration<double>(endTimer - startTimer).count();

    //! check vector
    startTimer = system_clock::now();
    vector<int64_t> data_v;
    data_v.reserve(size);
    endTimer = system_clock::now();

    cout << "Allocate with vector reserve: " << duration<double>(endTimer - startTimer).count() << endl;

    startTimer = system_clock::now();
    for (size_t i = 0; i < size; i++) {
        data_v[i] = i;
    }
    endTimer = system_clock::now();

    cout << "Fill Vector with ref: " << duration<double>(endTimer - startTimer).count() << endl;
    data_v.clear();
    vector<int64_t> data_v2;

    startTimer = system_clock::now();
    data_v2.reserve(size);
    endTimer = system_clock::now();

    cout << "Allocate with vector reserve: " << duration<double>(endTimer - startTimer).count() << endl;

    startTimer = system_clock::now();
    for (size_t i = 0; i < size; i++) {
        data_v2.push_back(i);
    }
    endTimer = system_clock::now();

    cout << "Fill Vector with push_back: " << duration<double>(endTimer - startTimer).count() << endl;
    data_v2.clear();

    startTimer = system_clock::now();
    vector<int64_t> data_v3(size);

    endTimer = system_clock::now();

    cout << "Allocate with constructor: " << duration<double>(endTimer - startTimer).count() << endl;

    startTimer = system_clock::now();
    for (size_t i = 0; i < size; i++) {
        data_v3[i] = i;
    }
    endTimer = system_clock::now();

    cout << "Fill Vector with ref: " << duration<double>(endTimer - startTimer).count() << endl;
    data_v2.clear();
}
