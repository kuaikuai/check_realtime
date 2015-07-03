#ifndef _REAL_MAIL_MAN_H_
#define _REAL_MAIL_MAN_H_
#include <string>
#include <vector>
using std::string;
using std::vector;
class Mailman
{
public:
    Mailman() {};
    void setServer(const string& server) {_smtpserver = server; };
    void setFrom(const string& from) { _from = from; };
    void setTo(vector<string>& to) { _to = to; };
    void setUsername(const string& username) { _username = username; }
    void setPassword(const string& password) { _password = password; }
    int sendmail(const string& subject, const string& body);
private:
    vector<string> _to;
    string _from;
    string _smtpserver;
    string _username;
    string _password;
};

#endif
