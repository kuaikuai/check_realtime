
#ifndef check_realtime_config_INCLUDED
#define check_realtime_config_INCLUDED

#ifdef __cplusplus
extern "C" 
{
#endif

int read_config_file(const char *config_file);
char *get_config_param(const char *name);
#ifdef __cplusplus
}  /* end extern "C" */
#endif


#endif /* INCLUDED */
