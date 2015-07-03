#include <string>
#include <iostream>
#include <time.h>
#include <sqlite3.h>
#include <stdio.h>
#include <inttypes.h> //for PRId64
#include "file_db.h"
#include "myexception.h"
#include "file_log.h"

using namespace file_check::log;
using std::string;
using std::cout;
using std::endl;

int execute(sqlite3 *conn, char* sql)
{
    int rc;
    char *errmsg = 0;
    string s = sql;
    //s += "; commit;";
    rc = sqlite3_exec(conn, s.c_str(), NULL, NULL, &errmsg);
    if(rc) {
        log_error("execute %s error: %s\n", s.c_str(), errmsg);
        if(errmsg) {
            sqlite3_free(errmsg);
        }
    }
    return (rc != SQLITE_OK)? -1: 0;
}

FileDB::FileDB(const string& path)
{
    int error = 0;
    _path = path;
    _conn = 0;

    if ((error = sqlite3_open(_path.c_str(), &_conn)) != 0) {
        MY_THROW(MyException, "sqlite3 open failed");
    }
    char sql[] = "CREATE TABLE IF NOT EXISTS files (name TEXT, md5sum TEXT, timestamp INTEGER)";
    if (execute(_conn, sql)) {
        MY_THROW(MyException, "init sqlite3 db failed");
    }
}

FileDB::~FileDB()
{
    if(_conn) {
        sqlite3_close(_conn);
    }
}


int FileDB::find(const string& name, string& md5sum, time_t & timestamp)
{
    char stmt[512];

    sqlite3_stmt *res;
    int error = 0;
    const char *tail;
    char *sum = 0;

    sprintf(stmt, "select md5sum, timestamp from files where name = \"%s\"", name.c_str());
    error = sqlite3_prepare_v2(_conn, stmt, 1000, &res, &tail);
    if (error == SQLITE_OK) {
        if (sqlite3_step(res) == SQLITE_ROW) {
            sum = (char *)sqlite3_column_text(res, 0);
            timestamp = sqlite3_column_int64(res, 1);
        }
    }
    if(0 == sum) {
        return -1;
    }

    md5sum.assign(sum);
    sqlite3_finalize(res);

    return 0;
}

int FileDB::insert(const string& name, string& md5sum)
{
    char sql[1024];
    time_t now = time(NULL);

    snprintf(sql, sizeof(sql), "insert into files (name, md5sum, timestamp) values ('%s', '%s', %ld)",
             name.c_str(), md5sum.c_str(), now);
    cout << sql << endl;
    return execute(_conn, sql);
}

int FileDB::update(const string& name, string& md5sum)
{
    char sql[1024];
    time_t now = time(NULL);
    snprintf(sql, sizeof(sql), "update files set md5sum = '%s', timestamp = %ld where name ='%s'", md5sum.c_str(), now, name.c_str());
    cout << sql << endl;
    return execute(_conn, sql);
}

int FileDB::del(const string&name)
{
    char sql[1024];
    snprintf(sql, sizeof(sql), "delete from files where name ='%s'", name.c_str());
    return execute(_conn, sql);
}
