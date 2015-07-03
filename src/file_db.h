#ifndef _FILE_MONITOR_DB_H_
#define _FILE_MONITOR_DB_H_

#include <string>
#include <time.h>
#include "sqlite3.h"

class FileDB {
public:
    FileDB(const std::string& path);
    ~FileDB();
    int find(const std::string& name, std::string& md5sum, time_t &timestamp);
    int insert(const std::string& name, std::string& md5sum);
    int update(const std::string& name, std::string& md5sum);
    int del(const std::string& name);
private:
    std::string _path;
    sqlite3 *_conn;
};

#endif
