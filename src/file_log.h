#ifndef _MYSQL_REPLICATION_REPLICATION_LOG_INCLUDED
#define _MYSQL_REPLICATION_REPLICATION_LOG_INCLUDED

#include <stdarg.h>
#include <fstream>
#include <string>
#include <pthread.h>

namespace file_check
{
    namespace log
    {
        #define LOG_LEVEL_STRING_LENGTH 10
        #define MAX_LOG_BUF_LENGTH  4048
        #define TIME_FORMAT_LENGTH  20

        enum LOGGER_LEVEL
        {
            LOGGER_ERROR = 0,
            LOGGER_WARNING,
            LOGGER_INFO,
            LOGGER_DEBUG,
            LOGGER_DETAIL
        };

        const char* level_name(LOGGER_LEVEL _level);

        class Formater
        {
        public:
            Formater();
            ~Formater();

            int formater(LOGGER_LEVEL _level,const char* _file, int line, const char* _format, va_list _valist);
            int open_file(const char* _file_name);

        private:
            char* curtime(char* _time);
            std::ofstream file_handler;
            std::string file_name;
            long total_size;
            pthread_mutex_t _mutex;
        };

        int init_log(const char* _file_name, LOGGER_LEVEL _level);

        int error(const char* _file, int _line, const char* _format, ...);
        int warning(const char* _file, int _line, const char* _format, ...);
        int info(const char* _file, int _line, const char* _format, ...);
        int debug(const char* _file, int _line, const char* _format, ...);
        int detail(const char* _file, int _line, const char* _format, ...);
        #define log_error(format, ...) error(__FILE__, __LINE__, format, ##__VA_ARGS__);
        #define log_warning(format, ...) warning(__FILE__, __LINE__, format, ##__VA_ARGS__);
    }
}
#endif
