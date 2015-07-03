#include "file_log.h"
#include <string.h>
#include <time.h>
#include <string>
#include <sstream>
#include <pthread.h>
using std::string;
using std::ostringstream;


class CGuard
{
  public:
  CGuard(pthread_mutex_t *mutex): _mutex(mutex) {
    pthread_mutex_lock(_mutex);
  }

  ~CGuard() {
    pthread_mutex_unlock(_mutex);
  }
  private:
  pthread_mutex_t* _mutex;
};


namespace file_check
{
    namespace log
    {
      template<class T>
      void to_string(string & result,const T& t)
      {
         ostringstream oss;
         oss << t;
         result = oss.str();
      }
        const char* level_name(LOGGER_LEVEL _level)
        {
            switch( _level )
            {
            case LOGGER_WARNING :
                return "WARNING";

            case LOGGER_INFO :
                return "INFO";

            case LOGGER_DETAIL :
                return "DETAIL";

            case LOGGER_DEBUG :
                return "DEBUG";

            case LOGGER_ERROR :
            default:
                return "ERROR";
            }
        }

        static Formater logger;
        static LOGGER_LEVEL  cur_level = LOGGER_ERROR;

        Formater::Formater(): total_size(0)
        {
          pthread_mutex_init(&_mutex,NULL);
        }

        Formater::~Formater()
        {
            if(file_handler.is_open())
                file_handler.close();
        }

        int Formater::open_file(const char* _file_name)
        {
            file_name = _file_name;
            file_handler.open(_file_name,std::ios::app|std::ios::out|std::ios::binary);
            if(!file_handler)
            {
                printf("open file %s error!\n", _file_name);
                return -1;
            }

            return  0;
        }

        char * Formater::curtime(char* _time)
        {
            time_t longtime = time(0);
            strftime(_time, TIME_FORMAT_LENGTH,"%Y/%m/%d %X %A",localtime(&longtime));

            return _time;
        }

        int Formater::formater(LOGGER_LEVEL _level,const char* _file, int _line, const char* _format, va_list _valist)
        {
            char buffer[MAX_LOG_BUF_LENGTH + 1] = {0};

            char str_level[LOG_LEVEL_STRING_LENGTH + 1] = {0};
            strncpy(str_level, level_name(_level), LOG_LEVEL_STRING_LENGTH);
            char time_str[TIME_FORMAT_LENGTH+1] = {0};
            int ret = snprintf(buffer, MAX_LOG_BUF_LENGTH, "%s [%s] : %s/%d : ", curtime(time_str), str_level, _file, _line);
            if( ret < 0 )
            {
                printf("snprintf is return negative value.ret is [%d]\n", ret);
                return -1;
            }

            ret = vsnprintf(buffer + strlen(buffer),  MAX_LOG_BUF_LENGTH - strlen(buffer), _format, _valist);
            if( ret < 0 )
            {
                printf("vsnprintf is return negative value.ret is [%d]\n", ret);
                return -1;
            }
            CGuard guard(&_mutex);
            total_size += ret;
            if(total_size > 1204*1204*10) {
                file_handler.close();
                time_t now = time(NULL);
                string str;
                to_string(str, now);
                rename(file_name.c_str(), (file_name + str).c_str());
                total_size = 0;
                file_handler.open(file_name.c_str(), std::ios::app|std::ios::out|std::ios::binary);
            }
            if(!file_handler.is_open())
            {
                printf("open file %s error!\n", file_name.c_str());
                return -1;
            }

            file_handler<<buffer;
            file_handler.flush();

            return 0;
        }

        int init_log(const char* _file_name, LOGGER_LEVEL _level)
        {
            cur_level = _level;
            return logger.open_file(_file_name);
        }

        int detail(const char* _file, int _line, const char* _format, ...)
        {
            if( LOGGER_DETAIL > cur_level)
                return 0;

            va_list ap;
            va_start(ap,_format);

            return logger.formater(LOGGER_DETAIL, _file, _line, _format, ap);
        }

        int debug(const char* _file, int _line,const char* _format, ...)
        {
            if( LOGGER_DEBUG > cur_level)
                return 0;

            va_list ap;
            va_start(ap,_format);

            return logger.formater(LOGGER_DEBUG, _file, _line, _format, ap);
        }

        int info(const char* _file, int _line,const char* _format, ...)
        {
            if( LOGGER_INFO > cur_level)
                return 0;

            va_list ap;
            va_start(ap,_format);

            return logger.formater(LOGGER_INFO, _file, _line, _format, ap);
        }

        int warning(const char* _file, int _line,const char* _format, ...)
        {
            if( LOGGER_WARNING > cur_level)
                return 0;

            va_list ap;
            va_start(ap,_format);

            return logger.formater(LOGGER_WARNING, _file, _line, _format, ap);
        }

        int error(const char* _file, int _line, const char* _format, ...)
        {
            if( LOGGER_ERROR > cur_level)
                return 0;

            va_list ap;
            va_start(ap,_format);

            return logger.formater(LOGGER_ERROR, _file, _line, _format, ap);
        }
    }
}

