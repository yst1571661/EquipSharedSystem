#ifndef TEST_LOG_H_
#define TEST_LOG_H_
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>

#define PTHREAD_SAFE_

#define LOG_ENABLE

#define FLUSH_LEVEL   LOG_EMERG

#define LOG_ERROR               0
#define LOG_EMERG               1
#define LOG_WANRING             2
#define LOG_NOTICE              3
#define LOG_DEBUG               4

#define DEBUG_ENABLE

#define DEBUG_CORE            5
#define DEBUG_NET             6

#define DEBUG_LAST            7

#define MAX_LOG     (1048576 << 0)       //1MB
#define MAX_LOG_COUNT    10
#define MAX_NAME    50
#define MAX_ERROR_STR 2048

#define LOGFILETMPDIR   "/tmp/local.log"
#define LOGFILEBACKDIR  "/mnt/log/local.log"
typedef struct abt_log log_t;
unsigned LogNum;


struct abt_log {
    int log_level;

    int fd;
    uint32_t size;
    FILE *fp;
    int std;
    char file[MAX_NAME];
};


#define         WARN        7


#ifdef LOG_ENABLE
#define init_abt_log(filename, level) init_log(filename, level)//初始化，参数时文件名和特定功能对应的编号
#define close_abt_log()               close_log()
#else
#define init_abt_log(filename, level)
#define close_abt_log()
#endif

int init_log(char *filename, int level);
void close_log();

#ifdef LOG_ENABLE
#define log_error(level, fmt, args...)                                        \
    log_error_core(level, __FILE__, __FUNCTION__, __LINE__, fmt, ##args)
#else
    #define log_error(level, fmt, ...)
#endif

#ifdef DEBUG_ENABLE
#define log_debug(level, fmt, args...)                                        \
    log_error_core(level, __FILE__,__FUNCTION__, __LINE__,fmt, ##args)
#else
    log_debug(level, fmt, ...)
#endif

void log_error_core(int level , const char *file,const char *fun, int line, const char *fmt, ...);





















#endif
