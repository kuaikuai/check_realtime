#ifndef _FILE_ALERT_H_
#define _FILE_ALERT_H_
#include <string>
#include <tr1/unordered_map>
#include <tr1/functional>
#include <utility>
#include <vector>

typedef std::pair<std::string, int> dir_level;
typedef std::tr1::unordered_map< dir_level, std::vector<std::string> > conf_map;
typedef std::tr1::function <int (int, const std::string&, const std::string)> action_function;
typedef std::tr1::unordered_map< std::string, action_function > action_map;

#if 1
namespace std {
    namespace tr1 {
        template <>
            struct hash< std::pair<string, int> >
        {
            std::size_t operator()(const std::pair<string, int>& k) const
            {
                return ((hash<string>()(k.first)
                         ^ (hash<int>()(k.second) << 1)) );
            }
        };
    }
}
#endif
class Alerter {
public:
    enum { CHANGED = 1, CREATED = 2, DELETED = 3 };
    void add_rule(int level, const std::string& dir, std::vector<std::string>& type);
    int find_rule(int level, const std::string& dir, std::vector<std::string>& type);
    void add_action(const std::string& dir, action_function act);
    void alert(int level, const std::string& dir, const std::string& name);
private:
    conf_map _conf;
    action_map _actions;
};
#endif
