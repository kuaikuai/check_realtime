#include <string>
#include <algorithm>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "md5.h"
#include <sstream>
#include <linux/limits.h>
#include <dirent.h>
#include <iostream>
#include <regex.h>
#include <tr1/functional>
#include <deque>
#include <tr1/memory>
#include <assert.h>
#include "file_db.h"
#include "myexception.h"
#include "util.h"
#include "file_log.h"
#include "file_analysis.h"

using namespace std;
using namespace file_check::log;
using std::tr1::placeholders::_1;

//#define MAX_LINE 512

#define MD5_DB_NAME "/usr/local/fountain/bin/windows.dll"
static int64_t global_id = 0;

static std::deque<analysis_node> anlysis_queue;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

class CGuard
{
  public:
  CGuard(pthread_mutex_t *mutex): _mutex(mutex) {
    pthread_mutex_lock(_mutex);
  }

  ~CGuard() {
    pthread_mutex_unlock(_mutex);
  }
  private:
  pthread_mutex_t* _mutex;
};

void add_queue(const analysis_node& node)
{
    pthread_mutex_lock(&mutex);
    anlysis_queue.push_back(node);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}

FileAnalysis::FileAnalysis()
{
    _fileDB.reset(new FileDB(MD5_DB_NAME));
}

void FileAnalysis::analysis(const string& base_dir, const string& final_name)
{
    cout << "Pass " << final_name << " to the judge." << endl;
    analysis_node node = { base_dir, final_name };
    add_queue(node);
}

inline void add_regex(regmap& regmap, const string& dir, const string& reg)
{
    string str = reg;
    stringtrim(str);
    if(str.length() == 0) {
        return;
    }
    regmap[dir].push_back(str);
}

void FileAnalysis::add_filter(const string& dir, strvec& regvec)
{
    for_each(regvec.begin(), regvec.end(), std::tr1::bind(add_regex, std::tr1::ref(_ignore), dir, _1));
}

void FileAnalysis::add_grep(const string& dir, strvec& regvec)
{
    for_each(regvec.begin(), regvec.end(), std::tr1::bind(add_regex, std::tr1::ref(_grep), dir, _1));
}

void FileAnalysis::loop()
{

    while(1) {

        pthread_mutex_lock(&mutex);
        while(anlysis_queue.size() == 0) {
            pthread_cond_wait(&cond, &mutex);
        }
        analysis_node node = anlysis_queue.front();
        anlysis_queue.pop_front();
        pthread_mutex_unlock(&mutex);

        do_analysis(node.dir, node.name);
    }

    return;
}

int FileAnalysis::get_task(const string& file, TaskTimer<analysis_node>& timer)
{
    CGuard guard(&mutex);

    if(_taskmap.count(file) == 0) {
        return -1;
    }
    timer = _taskmap[file];

    return 0;
}

void FileAnalysis::set_task(const string& file, const TaskTimer<analysis_node>& timer)
{
    CGuard guard(&mutex);
    _taskmap[file] = timer;
}

void FileAnalysis::del_task(const string& file)
{
    CGuard guard(&mutex);
    _taskmap.erase(file);
}

static void delay_check(const TaskTimer<analysis_node>* task)
{
    add_queue(task->user_data);
}

static bool is_prefix_(const string & s1, const string &s2)
{
    const char*p = s1.c_str();
    const char*q = s2.c_str();
    while (*p && *q)
        if (*p++ != *q++)
            return false;
    return true;
}
//todo: low preformance coz copy
static bool is_prefix(const string & s1, pair<string, strvec> it)
{
    return is_prefix_(s1, it.first);
}

static bool match(regmap& regs, const string& dir, const string& filename)
{
    regmap::iterator it = find_if(regs.begin(), regs.end(), std::tr1::bind(is_prefix, tr1::ref(filename), _1));
    if(it == regs.end()) {
        return false;
    }

    strvec& vec = it->second;
    if (count_if(vec.begin(), vec.end(), std::tr1::bind(match_regex, std::tr1::ref(filename), _1))) {
        return true;
    }
    return false;
}

void FileAnalysis::do_analysis(const string& base_dir, const string& final_name)
{
    cout << "Begin anlysising " << final_name << endl;
    // don't care
    if(match(_ignore, base_dir, final_name)) {
        cout << "Ignore " << final_name << endl;
        return;
    }
    // only care
    if(_grep.size() > 0 && !match(_grep, base_dir, final_name)) {
        cout << "not grep " << final_name << endl;
        return;
    }

    string checksum, sum;
    time_t timestamp;
    time_t now = time(NULL);
    /* this is a new file */
    if(_fileDB->find(final_name, sum, timestamp) != 0) {
        get_checksum(final_name, checksum);
        _fileDB->insert(final_name, checksum);
        _alerter.alert(Alerter::CREATED, base_dir, final_name);
        return;
    }

    if (_interval && now < timestamp + _interval) {
        cout << "Delay checking " << final_name << endl;
        TaskTimer<analysis_node> oldTask;
        if(get_task(final_name, oldTask) == 0) {
            TaskTimerManager<analysis_node>::getInstance().cancel(oldTask);
        }
        TaskTimer<analysis_node> taskTimer;
        //attention: global_id is not thread safe
        taskTimer.task_id = global_id++;
        taskTimer.callback = delay_check;
        taskTimer.expiration = _interval + now;
        analysis_node node = {base_dir, final_name};
        taskTimer.user_data = node;
        TaskTimerManager<analysis_node>::getInstance().add(taskTimer);
        set_task(final_name, taskTimer);
        return;
    }

    int rc = get_checksum(final_name, checksum);
    if(rc == FILE_NOT_EXIST) {
        _alerter.alert(Alerter::DELETED, base_dir, final_name);
        _fileDB->del(final_name);
    }
    else if(checksum != sum) {
        _fileDB->update(final_name, checksum);
        _alerter.alert(Alerter::CHANGED, base_dir, final_name);
    }
    del_task(final_name);

}
