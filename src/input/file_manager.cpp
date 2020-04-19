#include "input/file_manager.hpp"
#include <vector>

using namespace std;

bool file_exists(const string& name) {
    if (FILE* file = fopen(name.c_str(), "r")) {
        fclose(file);
        return true;
    } else {
        return false;
    }
}
