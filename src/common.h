#ifndef _FILE_REALTIME_COMMON_H_
#define _FILE_REALTIME_COMMON_H_
#include <string>
#include <vector>

typedef std::vector<std::string> string_vector;

struct item {
    string_vector paths;
    string_vector filters;
    string_vector actions;
};

#endif
