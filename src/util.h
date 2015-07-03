#ifndef _REAL_UTIL_H
#define _REAL_UTIL_H
#include <string>

#define FILE_NOT_EXIST -100

std::string &stringtrim(std::string &s);
std::string get_basedir (const std::string& str);
int get_checksum(const std::string& file_name, std::string &checksum);
bool match_regex(const std::string& str, const std::string& regex);
#endif
