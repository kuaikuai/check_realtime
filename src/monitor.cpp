/*
 * monitor the integerity of the files
 *  yxf 2014/08/24
 */


#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <linux/limits.h>
#include <libconfig.h++>
#include "file_watcher.h"
#include "myexception.h"
#include "mailman.h"
#include "file_log.h"

using namespace libconfig;
using std::cout;
using std::endl;
using namespace file_check::log;

#define OK 0
#define ERROR -1
#define ALERT_LOG_PATH "/var/log/zzhelper.log"
#define CFG_PATH "/usr/local/lib/libmoncfg.a"
#define CMD_FILE "/tmp/check_realtime.cmd"
#define PID_FILE "/tmp/check_realtime.pid"

std::string pid_file = PID_FILE;
using std::string;
Mailman mailman;

int read_proccess_name(pid_t pid, char *buff, int buff_len)
{
    int rc = 0;
    char temp[256];
    FILE *proc;

    buff[0] = '\0';
    sprintf(temp, "/proc/%d/comm", pid);

    if((proc = fopen(temp, "r")) == NULL) {
        return -1;
    }
    if(fgets(buff, buff_len, proc) == NULL) {
        rc = -1;
    }
    fclose(proc);
    return rc;
}

int get_pid(pid_t &pid)
{
    int fd;
    int len = 0;
    char pbuf[256];
    pid = 0;

    if((fd = open(pid_file.c_str(), O_RDONLY)) >= 0) {
        len = read(fd,pbuf,(sizeof pbuf)-1);
        if(len <= 0) {
            return -1;
        }
        close(fd);
        pbuf[len] = '\x0';
        // in SUSE11 64bit pid_t is int
        pid = (pid_t)atoi(pbuf);
        return 0;
    }
    return -1;
}

bool is_running(void)
{
    char buff[1024];
    pid_t pid;

    if(get_pid(pid) < 0) {
        return false;
    }
    if( kill(pid,0) < 0) {
        return false;
    }

    read_proccess_name(pid, buff, sizeof(buff));
    if(strstr(buff, "zzhelper") == NULL) {
        return false;
    }
    return true;
}

/* write an optional pid file */
int write_pid_file(void)
{
    int fd;
    pid_t pid=0;
    char pbuf[16];

    if(is_running()){
        syslog(LOG_ERR,"There's already an check_realtime server running (PID %lu).  Bailing out...",(unsigned long)pid);
        return ERROR;
    }

    if((fd = open(pid_file.c_str(), O_WRONLY | O_CREAT,0644)) >= 0){
        sprintf(pbuf,"%d\n",(int)getpid());
        write(fd,pbuf,strlen(pbuf));
        close(fd);
    }
    else{
        syslog(LOG_ERR,"Cannot write to pidfile '%s' - check your privileges.",pid_file.c_str());
    }

    return OK;
}

/* remove pid file */
int remove_pid_file(void) {

    if(unlink(pid_file.c_str())==-1){
        syslog(LOG_ERR,"Cannot remove pidfile '%s' - check your privileges.",pid_file.c_str());
        return ERROR;
    }

    return OK;
}

void call(const char *name)
{
}

int get_abspath(string &path)
{
    char buf[PATH_MAX];
    int n = readlink("/proc/self/exe", buf, PATH_MAX);
    if( n > 0 && n < PATH_MAX) {
        buf[n-1] = '\0';
        char *p = strrchr(buf, '/');
        if(NULL != p) {
            *p = '\0';
        }
        path = buf;
        return 0;
    }
    buf[0]='\0';
    return -1;
}

void setting2strvec(const Setting& setting, const string& name, strvec& vec)
{
    if(!setting.exists(name)) {
        return;
    }
    Setting &set = setting[name];
    // todo: trim
    for(int n = 0; n < set.getLength(); n++) {
        const char* s = set[n];
        vec.push_back(s);
    }
}

strvec& lower_first(strvec& low, strvec& high)
{
    if(low.size() != 0) {
        return low;
    }
    return high;
}

void* analysis_loop(void *arg)
{
    File_watcher *watcher = (File_watcher *)arg;
    watcher->get_judge().loop();
    return NULL;
}

int get_alert(int level, const string& dir, const string& name, string &alert)
{
    string body = "File " + name;
    if(level == Alerter::CHANGED) {
        body += " have been changed!!";
    }
    else if(level == Alerter::CREATED) {
        body += " have been created!!";
    }
    else if(level == Alerter::DELETED) {
        body += " have been deleted!!";
    }
    alert = body;
    return 0;
}

int send_mail(int level, const string& dir, const string& name)
{
    string subject = "A Warning of File Integerity Monitor";
    string body = "Attention please!\r\n";
    string alert;
    get_alert(level, dir, name, alert);
    if (mailman.sendmail(subject, body + alert) < 0) {
        log_warning("can not send mail!");
    }

    cout << "send mail " << dir << " " << name << endl;
    return 0;
}

int log_alert(int level, const string& dir, const string& name)
{
    string alert;
    get_alert(level, dir, name, alert);
    log_warning("%s\n", alert.c_str());
    cout << "log alert " << dir << " " << name << endl;
    return 0;
}


int init_action(Alerter& alerter)
{
    alerter.add_action("mail", send_mail);
    alerter.add_action("log", log_alert);
    return 0;
}

int load_mailman(const Setting& root)
{
    Setting &set = root["mail"];
    mailman.setServer(set["server"]);
    mailman.setFrom(set["from"]);
    strvec to;
    setting2strvec(set, "to", to);
    mailman.setTo(to);
    mailman.setUsername(set["username"]);
    mailman.setPassword(set["password"]);
    return 0;
}

void get_int_value(const Setting& root, const string& name, int &val)
{
    if(root.exists(name)) {
        val = root[name];
    }
}

void load_alerter(const Setting& root, Alerter& alerter)
{
    strvec file_created_alert, file_changed_alert, file_deleted_alert;
    setting2strvec(root, "file_created_alert", file_created_alert);
    setting2strvec(root, "file_changed_alert", file_changed_alert);
    setting2strvec(root, "file_deleted_alert", file_deleted_alert);

    const Setting &checks = root["checks"];
    int count = checks.getLength();
    for(int i = 0; i < count; ++i) {
        const Setting &check = checks[i];
        if(!check.exists("path")) {
            continue;
        }
        strvec check_created_alert;
        setting2strvec(check, "file_created_alert", check_created_alert);
        strvec& created_alert = lower_first(check_created_alert, file_created_alert);

        strvec check_changed_alert;
        setting2strvec(check, "file_changed_alert", check_changed_alert);
        strvec& changed_alert = lower_first(check_changed_alert, file_changed_alert);

        strvec check_deleted_alert;
        setting2strvec(check, "file_deleted_alert", check_deleted_alert);
        strvec& deleted_alert = lower_first(check_deleted_alert, file_deleted_alert);

        const Setting &paths = check["path"];
        for(int n = 0; n < paths.getLength(); n++) {
            string path = paths[n];
            alerter.add_rule(Alerter::CREATED, path, created_alert);
            alerter.add_rule(Alerter::CHANGED, path, changed_alert);
            alerter.add_rule(Alerter::DELETED, path, deleted_alert);
        }
    }
}

void load_judge(const Setting& root, FileAnalysis& judge)
{
    int interval = 0;
    get_int_value(root, "alert_interval", interval);
    // interval seconds
    judge.set_interval(interval*60);
    const Setting &checks = root["checks"];
    int count = checks.getLength();
    for(int i = 0; i < count; ++i) {
        const Setting &check = checks[i];
        if(!check.exists("path")) {
            continue;
        }
        strvec filter, grep;
        setting2strvec(check, "filter", filter);
        setting2strvec(check, "grep", grep);
        const Setting &paths = check["path"];
        for(int n = 0; n < paths.getLength(); n++) {
            string path = paths[n];
            judge.add_filter(path, filter);
            judge.add_grep(path, grep);
        }
    }
}

void load_excluded_dir(const Setting& root, File_watcher& watcher)
{
    const Setting &checks = root["checks"];
    int count = checks.getLength();
    for(int i = 0; i < count; ++i) {
        const Setting &check = checks[i];
        if(!check.exists("path")) {
            continue;
        }
        strvec filter;
        setting2strvec(check, "filter", filter);
        watcher.exclude_dir(filter);
    }
}

void load_watcher(const Setting& root, File_watcher& watcher)
{
    int sleep_interval = 0;
    int sleep_count = 0;


    load_excluded_dir(root, watcher);

    get_int_value(root, "sleep_interval", sleep_interval);
    get_int_value(root, "sleep_count", sleep_count);

    const Setting &checks = root["checks"];
    int count = checks.getLength();
    for(int i = 0; i < count; ++i) {
        const Setting &check = checks[i];
        if(!check.exists("path")) {
            continue;
        }
        int recursive = 0;
        check.lookupValue("recursive", recursive);
        const Setting &paths = check["path"];
        for(int n = 0; n < paths.getLength(); n++) {
            string path = paths[n];
            watcher.add_dir(path, recursive, sleep_count, sleep_interval);
        }
    }
}

int load_conf(File_watcher& watcher)
{
    Config cfg;
    try {
        cfg.readFile(CFG_PATH);
        const Setting& root = cfg.getRoot();
        load_mailman(root);
        FileAnalysis& judge = watcher.get_judge();
        Alerter &alerter = judge.get_alerter();
        load_alerter(root, alerter);
        load_judge(root, judge);
        load_watcher(root, watcher);
    }
    catch(MyException &e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
    catch(const FileIOException &fioex)
    {
        std::cerr << "I/O error while reading file." << std::endl;
        return -1;
    }
    catch(const ParseException &pex)
    {
        std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
                  << " - " << pex.getError() << std::endl;
        return -1;
    }
    catch(const SettingNotFoundException &nfex) {
        return -1;
    }
    return 0;
}

void *run_timer_loop(void *param)
{
    TaskTimerManager<analysis_node>::getInstance().execute(NULL);
    return NULL;
}

int main(int argc, char *argv[])
{
    int is_daemon = 1;
    int skip = 0;
    if(argc < 2) {
        return -1;
    }
    init_log(ALERT_LOG_PATH, LOGGER_WARNING);

    if(strcmp("--test", argv[1]) == 0) {
        is_daemon = 0;
    }
    else if(strcmp("--stop", argv[1]) == 0) {
        if(!is_running()) {
            return 0;
        }
        File_watcher::stop(CMD_FILE);
        pid_t pid;
        if(get_pid(pid) < 0) {
            printf("failed to stop\n");
            return -1;
        }
        kill(pid, SIGTERM);
        return 0;
    }
    else if(strcmp("--start", argv[1]) == 0) {
        if(is_running()) {
            printf("running...\n");
            return 0;
        }
    }
    else {
        /* invalid command */
        return 0;
    }
    if (argc >= 3 && strcmp("--skip", argv[2]) == 0) {
        skip = 1;
    }
    if(is_daemon) {
        daemon(0,0);
    }
    write_pid_file();

    File_watcher* watcher;
    try {
        watcher = new File_watcher(CMD_FILE, skip);
    } catch (MyException &e) {
        log_error("found error: %s", e.what());
        std::cerr << e.what() << std::endl;
        return -1;
    }

    Alerter& alerter = watcher->get_judge().get_alerter();
    init_action(alerter);
    if (load_conf(*watcher) < 0) {
        log_error("error load conf");
        return -1;
    }
    pthread_t pid;

    pthread_create(&pid, NULL, analysis_loop, watcher);
    pthread_create(&pid, NULL, run_timer_loop, NULL);
    watcher->loop();

    remove_pid_file();
    return 0;
}
