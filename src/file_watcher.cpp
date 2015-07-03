
/*
  monitor the integerity of the files
*/

#include <string>
#include <algorithm>
#include "file_watcher.h"
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

using namespace file_check::log;

using std::cout;
using std::string;
using std::stringstream;

using std::tr1::placeholders::_1;
using namespace std;

File_watcher::File_watcher(const string& command_file, int skip)
    :_command_file(command_file), _skip(skip)
{
    _fd = inotify_init();
    unlink(_command_file.c_str());

    int result;
    if((result = mkfifo(_command_file.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)) != 0) {
        log_error("mkfifo %s error", _command_file.c_str());
        return;
    }

    if(_fd < 0) {
        log_error("Unable to initialize inotify.");
        return;
    }
}

File_watcher::~File_watcher()
{
    unlink(_command_file.c_str());
}

int File_watcher::stop(const string& command_file)
{
    int command_file_fd;
    if((command_file_fd = open(command_file.c_str(), O_RDWR | O_NONBLOCK)) < 0) {
        log_error("open command file failed");
        return -1;
    }
    write(command_file_fd, "1", 1);
    close(command_file_fd);
    return 0;
}

bool is_dir(const string& file_name)
{
    struct stat statbuf;
    if(lstat(file_name.c_str(), &statbuf) < 0) {
        return -1;
    }
    if(S_ISDIR(statbuf.st_mode)) {
        return true;
    }
    return false;
}

struct Entry {
    int type;
    string name;
};

typedef std::tr1::function<bool (const string&)> fun_t;

class EntryIterator
{
public:
    EntryIterator(const string& dir, fun_t is_interestring ,int recursive);
    bool is_end();
    Entry next();
private:
    std::deque<string> _queue;
    fun_t _is_interestring;
    int _recursive;
};

EntryIterator::EntryIterator(const string& dir, fun_t is_interestring, int recursive)
    :_is_interestring(is_interestring), _recursive(recursive)
{
    _queue.push_back(dir);
}

bool EntryIterator::is_end()
{
    return _queue.size() == 0;
}

Entry EntryIterator::next()
{
    char f_name[PATH_MAX +2];
    DIR *dp;
    struct dirent *entry;

    while(_queue.size() > 0) {
        int type = 0;
        string dir_name = _queue.back();
        _queue.pop_back();

        f_name[PATH_MAX +1] = '\0';
        if(is_dir(dir_name)) {
            type = 1;
            if(!_is_interestring(dir_name)) {
                continue;
            }
            dp = opendir(dir_name.c_str());
            if(!dp) {
                cout << "opendir " << dir_name << " error:"<<strerror(errno) << std::endl;
                continue;
            }

            while((entry = readdir(dp)) != NULL) {
                char *s_name;

                if((strcmp(entry->d_name,".") == 0) ||
                   (strcmp(entry->d_name,"..") == 0))
                    continue;

                strncpy(f_name, dir_name.c_str(), PATH_MAX);
                s_name = f_name;
                s_name += dir_name.length();

                if(*(s_name-1) != '/')
                    *s_name++ = '/';

                *s_name = '\0';
                strncpy(s_name, entry->d_name, PATH_MAX - dir_name.length() -2);
                if (!is_dir(f_name) || _recursive) {
                    _queue.push_back(f_name);
                }
            }
            closedir(dp);
        }
        Entry dentry = { type, dir_name };
        return dentry;
    }
    // todo
    Entry dentry = { 1, "" };
    return dentry;
}

time_t get_file_timestamp (const string& name)
{
    struct stat statbuf;
    int rc = stat(name.c_str(), &statbuf);
    if(0 == rc) {
        return statbuf.st_mtime;
    }
    return 0;
}

void File_watcher::exclude_dir(strvec& filter)
{
    for(strvec::iterator it = filter.begin();
        it != filter.end(); ++it) {
        if (it->length() == 0) {
            continue;
        }
        _filter.push_back(*it);
    }
}

bool File_watcher::need_mon(const string& dir)
{
    strvec::iterator it = find(_filter.begin(), _filter.end(), dir);
    return (it == _filter.end()); 
}

int File_watcher::add_dir(const string& dir, int recursive, int sleep_count, int sleep_interval)
{
    EntryIterator it(dir, std::tr1::bind(&File_watcher::need_mon, this, _1), recursive);
    int count = 0;
    while(1) {
        Entry entry = it.next();
        if(entry.name.length() == 0) {
            break;
        }
        /* dir */
        if(entry.type == 1) {
            add_dir(entry.name);
        }
        else { //file
            // do not read files
            if(_skip) {
                continue;
            }
            count++;
            if (sleep_count && sleep_interval
                && count % sleep_count == 0) {
                sleep(sleep_interval);
            }
            get_judge().do_analysis(get_basedir(entry.name), entry.name);
        }
    }
    return 0;
}

int File_watcher::add_dir(const string& dir)
{
    if(_fd < 0) {
        return(-1);
    }
    else {
        int wd = 0;
        cout << "Prepare for monitoring dir:" << dir << std::endl;
        wd = inotify_add_watch(_fd, dir.c_str(), REALTIME_MONITOR_FLAGS);
        if(wd < 0) {
            cout << "inotify_add_watch error:"<<strerror(errno)<<std::endl;
            return -1;
        }
        else {
            hashmap::iterator it = _hash.find(wd);
            if(it == _hash.end()) {
                _hash[wd] = dir;
            }
        }
    }

    return 0;
}

void File_watcher::loop()
{
    struct pollfd pfd[2];
    int command_file_fd;

    pfd[0].fd = _fd;
    pfd[0].events = POLLIN | POLLERR;
    if((command_file_fd = open(_command_file.c_str(), O_RDWR | O_NONBLOCK)) > 0) {
        pfd[1].fd = command_file_fd;
        pfd[1].events = POLLIN | POLLERR;
    }
    else {
        log_error("loop: open command file failed");
    }
    do {
        int rt = poll(pfd, 2, -1);
        if(-1 == rt) {
            if (errno == EINTR)
                continue;
            else {
                printf("poll fail: %s\n", strerror(errno));
                continue;
            }
        }
        if(pfd[0].revents & POLLIN) {
            watch();
        }
        if(pfd[1].revents & (POLLIN | POLLERR) || (pfd[0].revents & POLLERR)) {
            log_error("poll found error!");
            break;
        }
    } while(1);
}

void File_watcher::watch()
{
    int len, i = 0;
    char buf[REALTIME_EVENT_BUFFER +1];
    struct inotify_event *event;

    buf[REALTIME_EVENT_BUFFER] = '\0';

    len = read(_fd, buf, REALTIME_EVENT_BUFFER);
    if (len < 0) {
        return;
    }
    else if (len > 0) {
        for (i = 0; i < len; i += REALTIME_EVENT_SIZE + event->len) {
            event = (struct inotify_event *) &buf[i];
            if(event->len <= 0) {
                continue;
            }

            hashmap::iterator it = _hash.find(event->wd);
            const string& base_dir = it->second;
            string final_name;
            final_name.append(base_dir).append("/").append(event->name);
            cout << "Find change of " << final_name << endl;
            _judge.analysis(base_dir, final_name);
        }
    }
}
