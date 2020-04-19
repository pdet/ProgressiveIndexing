#include <chrono>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>
#include <algorithm>

using namespace std;
using namespace chrono;

struct col{
    int64_t key;
    int64_t id;
};

int main(int argc, char** argv) {
    time_point<system_clock> startTimer, endTimer;

    int size = pow(10, 8);
    int64_t* data;
    data = (int64_t*)malloc(sizeof(int64_t) * size);
    col* column = (col*)malloc(sizeof(col) * size);
    for (size_t i = 0; i < size; i++) {
        data[i] = i;
    }
    shuffle(data, data + size, default_random_engine(1));
    for (size_t i = 0; i < size; i++) {
        column[i].id = i;
        column[i].key = data[i];
    }
    int64_t low = 0;
    int64_t high = size;
    int64_t sumMatch = 0, sumScan = 0;

    double matchTime = 0, noMatchTime = 0;
    for (size_t j = 0; j < 10; j++) {
        startTimer = system_clock::now();
        for (size_t i = 0; i < size; i++) {
            if (data[i] >= low && data[i] < high) {
                sumMatch += data[i];
            }
        }
        endTimer = system_clock::now();
        matchTime += duration<double>(endTimer - startTimer).count();

        startTimer = system_clock::now();
        for (size_t i = 0; i < size; i++) {
            sumScan += data[i];
        }
        endTimer = system_clock::now();
        noMatchTime += duration<double>(endTimer - startTimer).count();

        if (sumMatch != sumScan) {
            cout << "Life is not so good sometimes" << endl;
        }
    }
    matchTime /= 10;
    noMatchTime /= 10;

    cout << "Scan with match: " << matchTime << endl;
    cout << "Scan wout match: " << noMatchTime << endl;
    auto tot = (matchTime + noMatchTime);
    auto diff = (matchTime - noMatchTime);
    auto percent = ((diff / (tot)) * 100);
    if (percent < 0) {
        percent = ((percent - percent) - percent);
    }
    cout << "Percent difference: " << percent << endl;

    matchTime = 0;
    noMatchTime = 0;
    for (size_t j = 0; j < 10; j++) {
        startTimer = system_clock::now();
        for (size_t i = 0; i < size; i++) {
            if (column[i].key >= low && column[i].key < high) {
                sumMatch += data[i];
            }
        }
        endTimer = system_clock::now();
        matchTime += duration<double>(endTimer - startTimer).count();

        startTimer = system_clock::now();
        for (size_t i = 0; i < size; i++) {
            sumScan += column[i].key;
        }
        endTimer = system_clock::now();
        noMatchTime += duration<double>(endTimer - startTimer).count();

        if (sumMatch != sumScan) {
            cout << "Life is not so good sometimes" << endl;
        }
    }
    matchTime /= 10;
    noMatchTime /= 10;

    cout << "Scan with match: " << matchTime << endl;
    cout << "Scan wout match: " << noMatchTime << endl;
     tot = (matchTime + noMatchTime);
     diff = (matchTime - noMatchTime);
     percent = ((diff / (tot)) * 100);
    if (percent < 0) {
        percent = ((percent - percent) - percent);
    }
    cout << "Percent difference: " << percent << endl;
}
