#include <stdio.h>
#include <string>
#include <vector>
#include <tr1/functional>
#include <iostream>
#include "util.h"
#include "alerter.h"

using std::string;
using std::vector;
using std::cout;
using std::endl;

void Alerter::alert(int level, const string& dir, const string& name)
{
    cout << "Alert " << level << " " << name << endl;
    int type = -1;
    vector<string> action;
    string old, path = dir;
    do {
        cout << "Try to find dir:" << path << " name:" << name << endl;
        if((type = find_rule(level, path, action)) < 0) {
            old = path;
            path = get_basedir(old);
        }
        else {
            break;
        }
    } while(old != path);
    if(type < 0) {
       cout << "None alert rules for " << name << endl;
       return;
    }
    for(vector<string>::iterator it = action.begin();
        it != action.end(); it++) {
        if(_actions.count(*it)) {
            _actions[*it](level, dir, name);
        }
    }
}

void Alerter::add_action(const string& type, action_function act)
{
    _actions[type] = act;
}

void Alerter::add_rule(int level, const string& dir, vector<string>& type)
{
    string path = dir;
    dir_level dlevel(dir, level);

    stringtrim(path);
    if(path.length() == 0) {
        return;
    }
    cout << "alerter:: add rule level="<<level << " dir=" << dir << " type=";
    for(vector<string>::iterator it = type.begin(); it != type.end(); ++it) {
        cout << *it;
    }
    cout << endl;
   _conf[dlevel] = type;
}


int Alerter::find_rule(int level, const string& dir, vector<string>& type)
{
    dir_level dlevel(dir, level);

    conf_map::iterator it = _conf.find(dlevel);
    if(it != _conf.end()) {
        type = it->second;
        return 0;
    }

    return -1;
}
