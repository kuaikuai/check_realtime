#include <algorithm>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sstream>
#include <linux/limits.h>
#include <dirent.h>
#include <iostream>
#include <sstream>
#include <regex.h>
#include <tr1/functional>
#include <deque>
#include <tr1/memory>
#include <assert.h>
#include "md5.h"
#include "util.h"

using namespace std;

// trim from start
static inline string &ltrim(string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

// trim from end
static inline string &rtrim(string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

// trim from both ends
string &stringtrim(string &s)
{
    return ltrim(rtrim(s));
}

string get_basedir (const string& str)
{
    size_t found;
    found=str.find_last_of("/\\");
    return str.substr(0,found);

}

struct checksum_struct {
    char str[33];
};

static int get_md5sum(const string& fname, checksum_struct *checksum)
{
    md5_state_t state;
    md5_byte_t digest[16];
    char hex_output[16*2 + 1];
    md5_byte_t buff[1024];
    int fd = open(fname.c_str(), O_RDONLY);
    int di,rc;

    checksum->str[0] = '\0';
    if(fd < 0) {
        printf("open file %s failed, error %s\n", fname.c_str(), strerror(errno));
        return -1;
    }
    md5_init(&state);

    while((rc = read(fd, buff, sizeof(buff))) > 0) {
        md5_append(&state, buff, rc);
    }
    if(rc < 0) {
        printf("read file %s failed, error %s\n", fname.c_str(), strerror(errno));
        return -1;
    }
    md5_finish(&state, digest);
    for (di = 0; di < 16; ++di) {
        sprintf(hex_output + di * 2, "%02x", digest[di]);
    }
    memcpy(checksum, hex_output, sizeof(hex_output));
    close(fd);
    return 0;
}

int get_checksum(const string& file_name, string &checksum)
{
    struct stat statbuf;
    if(lstat(file_name.c_str(), &statbuf) < 0) {
        if (errno == ENOENT) {
            return FILE_NOT_EXIST;
        }
        return -1;
    }
    if(S_ISREG(statbuf.st_mode) || S_ISLNK(statbuf.st_mode)) {
        checksum_struct sum;
        get_md5sum(file_name, &sum);
        stringstream ss;
        ss << (int)statbuf.st_size << ":"
           << (int)statbuf.st_mode << ":"
           << (int)statbuf.st_uid << ":"
           << (int)statbuf.st_gid << ":"
           << sum.str;
        checksum = ss.str();
        return 0;
    }
    return -2;
}

/*
 * Compile a posix regex, returning NULL on error
 * Returns 1 if matches, 0 if not.
 */
bool match_regex(const string& str, const string& regex)
{
    regex_t preg;
    //std::cout << "match_regex: str:" << str << " regex:" << regex << std::endl;
    if(regcomp(&preg, const_cast<char *>(regex.c_str()), REG_EXTENDED|REG_NOSUB) != 0) {
        //merror("%s: Posix Regex compile error (%s).", __local_name, regex);
        return false;
    }

    if(regexec(&preg, const_cast<char *>(str.c_str()), str.length(), NULL, 0) != 0) {
        regfree(&preg);
        return false;
    }

    regfree(&preg);
    return true;
}
