#include <iostream>
#include <tr1/unordered_map>
#include <string>
#include <iostream>
#include <set>
#include <vector>
#include <sys/inotify.h>
#include <poll.h>
#include <tr1/memory>
#include <tr1/unordered_map>
#include "file_db.h"
#include "alerter.h"
#include "TaskTimerManager.hpp"

typedef std::vector<std::string> strvec;
typedef std::tr1::unordered_map< std::string, strvec > regmap;

struct analysis_node
{
    std::string dir;
    std::string name;
};

class FileAnalysis {
public:
    FileAnalysis();
    //~FileAnalysis();
    void loop();
    void add_filter(const std::string& dir, strvec& reg);
    void add_grep(const std::string& dir, strvec& reg);
    void analysis(const std::string& base_dir, const std::string& final_name);
    void do_analysis(const std::string& base_dir, const std::string& final_name);
    void set_interval(int interval) { _interval = interval; }
    Alerter &get_alerter() { return _alerter; }

private:
    int get_task(const std::string& file, TaskTimer<analysis_node>& timer);
    void set_task(const std::string& file, const TaskTimer<analysis_node>& timer);
    void del_task(const std::string& file);
    int _interval;
    regmap _ignore;
    regmap _grep;
    Alerter _alerter;
    std::tr1::shared_ptr<FileDB> _fileDB;
    std::tr1::unordered_map<std::string, TaskTimer<analysis_node> > _taskmap;
};
