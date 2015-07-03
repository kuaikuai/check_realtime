#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <stdio.h>
#include <unistd.h>
#include <tr1/functional>
#include "message_queue.h"

using std::string;
using std::tr1::placeholders::_1;
using std::tr1::placeholders::_2;

#define MAX_RETRY 3
static int bind_unix_domain(char* path, int mode, int max_msg_size);
static int connect_unix_domain(char * path, int max_msg_size);

bool file_is_exist(string file)
{
    struct stat file_status;

    if(stat((char *)file.c_str(), &file_status) < 0)
        return false;

    return true;
}

static int try_call(std::tr1::function <int ()> call)
{
    int i;
    for(i = 0; !call() && i < MAX_RETRY; i++) {
        sleep(1 + 5*i);
    }
    if(i >= MAX_RETRY) {
        return -1;
    }
    return 0;
}

int MQ::start()
{
    if(_type == READ) {
        _sock = bind_unix_domain((char *)_path.c_str(), 0660, 1024 + 512);
    }
    else {
        int rc = 0;
        rc = try_call(std::tr1::bind(file_is_exist, _path));
        if(rc < 0) {
             return -1;
        }
        _sock = try_call(std::tr1::bind(connect_unix_domain, (char *)_path.c_str(), 1024 + 512));
    }
    return _sock;
}

int MQ::send()
{
    return 0;
}

int MQ::recv()
{
    return 0;
}


static int bind_unix_domain(char* path, int mode, int max_msg_size)
{
    int len;
    int sock = 0;
    socklen_t optlen = sizeof(len);
    struct sockaddr_un n_us;
    socklen_t us_l = sizeof(n_us);

    /* Making sure the path isn't there */
    unlink(path);

    memset(&n_us, 0, sizeof(n_us));
    n_us.sun_family = AF_UNIX;
    strncpy(n_us.sun_path, path, sizeof(n_us.sun_path)-1);

    if((sock = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
        return -1;

    if(bind(sock, (struct sockaddr *)&n_us, sizeof(struct sockaddr_un)) < 0) {
        close(sock);
        return -1;
    }

    chmod(path,mode);

    if(getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &len, &optlen) == -1)
        return -1;

    if(len < max_msg_size) {
        len = max_msg_size;
        setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &len, optlen);
    }

    return sock;
}

static int connect_unix_domain(char * path, int max_msg_size)
{
    int len;
    int sock = 0;
    socklen_t optlen = sizeof(len);
    struct sockaddr_un n_us;
    socklen_t us_l = sizeof(n_us);

    memset(&n_us, 0, sizeof(n_us));
    n_us.sun_family = AF_UNIX;

    strncpy(n_us.sun_path,path,sizeof(n_us.sun_path)-1);

    if((sock = socket(AF_UNIX, SOCK_DGRAM,0)) < 0)
        return -1;

    if(connect(sock,(struct sockaddr *)&n_us,SUN_LEN(&n_us)) < 0)
        return -1;

    if(getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &len, &optlen) == -1)
        return -1;

    /* Setting maximum message size */
    if(len < max_msg_size)
    {
        len = max_msg_size;
        setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &len, optlen);
    }

    return(sock);
}

static int getsocketsize(int sock)
{
    int len = 0;
    socklen_t optlen = sizeof(len);

    /* Getting current maximum size */
    if(getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &len, &optlen) == -1)
        return -1;

    return(len);
}
