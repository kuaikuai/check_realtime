
#include <tr1/unordered_map>
#include <string>
#include <iostream>
#include <set>
#include <vector>
#include <sys/inotify.h>
#include <poll.h>
#include <tr1/memory>
#include <tr1/unordered_map>
#include "file_analysis.h"

#define REALTIME_MONITOR_FLAGS  IN_MODIFY|IN_ATTRIB|IN_MOVED_FROM|IN_MOVED_TO|IN_CREATE|IN_DELETE|IN_DELETE_SELF
#define REALTIME_EVENT_SIZE     (sizeof (struct inotify_event))
#define REALTIME_EVENT_BUFFER   (2048 * (REALTIME_EVENT_SIZE + 16))

typedef void (*watch_call_t)(const char *name);
typedef std::tr1::unordered_map< int, std::string > hashmap;
typedef std::tr1::unordered_map< std::string, std::string > checkmap;
typedef std::vector<std::string> namevector;

class File_watcher {
public:
    File_watcher(const std::string& command_file, int skip);
    ~File_watcher();
    int add_dir(const std::string& dir);
    int add_dir(const std::string& dir, int recursive, int sleep_count, int sleep_interval);
    void watch();
    void loop();
    void exclude_dir(strvec& filter);
    static int stop(const std::string& command_file);
    FileAnalysis& get_judge() { return _judge; };
private:
    bool need_mon(const std::string& dir);
    void read_dir(const std::string& dir, int recursive);
    File_watcher(File_watcher&); //forbidden
    File_watcher &operator=(File_watcher&); //forbidden
    int _fd;
    hashmap _hash;
    FileAnalysis _judge;
    std::string _command_file;
    int _skip;
    std::tr1::shared_ptr<FileDB> _fileDB;
    strvec _filter;
};
