#ifndef MY_MESSAGE_QUEUE_H
#define MY_MESSAGE_QUEUE_H

#include <string>
#define READ 1

class MQ {
public:
    int start();
    int send();
    int recv();
private:
    int _sock;
    std::string _path;
    int _type;
};

#endif
