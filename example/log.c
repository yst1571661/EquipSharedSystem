#include "log.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <time.h>

#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

#define PTHREAD_SAFE_

#define MAX_INFO_STR  128

#ifdef PTHREAD_SAFE_
pthread_mutex_t file_lock = PTHREAD_MUTEX_INITIALIZER;

#define mutex_lock()      (pthread_mutex_lock(&file_lock))
#define mutex_unlock()    (pthread_mutex_unlock(&file_lock))

#else
#define mutex_lock()
#define mutex_unlock()
#endif

static struct abt_log *log = NULL;

static const char*log_table[] = {"ERROR", "EMERG", "WANRING", "NOTICE", "DEBUG",
                                            "CORE", "NET"};


int init_log(char *filename, int level)
{
    int fd;
    uint32_t curPos;
    int std = 0;
    if (log != NULL)
        return -1;

    log = malloc(sizeof(struct abt_log));

    if (log == NULL) {
        perror("malloc");
        return -1;
    }
    log->std = 0;
    if (filename != NULL) {
		do {
        	fd = open(filename, O_RDWR | O_CREAT | O_APPEND );
			if (fd < 0 && errno == EINTR) {
				continue;
			} else
				break;
		} while (1);
        memcpy(log->file, filename, strlen(filename)+1);
    }
    else {
        log->file[0] = 0;
        fd = 1;
        log->std = 1;
        std = 1;
    }

    if (fd < 0) {
        perror("\nlog file open\n");
        free(log);
        log = NULL;
        return -1;
    }

    log->fd = fd;
    if (!std) {
        log->fp = fdopen(fd, "w+");

        if (log->fp == NULL) {
            perror("\nlog file stream\n");
            free(log);
            log = NULL;
            close(fd);
            return -1;
        }
    } else {
        log->fp = stdout;
    }
    log->log_level = level;
    if (!std) {
        curPos = ftell(log->fp);

        if (fseek(log->fp, 0, SEEK_END) < 0) {
            perror("\nlog file fseek\n");
            return -1;
        }

        log->size = ftell(log->fp);

        if (fseek(log->fp, curPos, SEEK_SET) < 0) {
            perror("\nlog file fseek\n");
            return -1;
        }
    }
    return 0;
}

void close_log()
{
    if (log != NULL) {
        fflush(log->fp);
        if (!log->std) {
            fclose(log->fp);
        }
        free(log);
        log = NULL;
    }
    return;
}

void log_error_core(int level , const char *file, const char *fun, int line, const char *fmt, ...)
{
    log_t *logs = log;
    int count = 0;
    va_list  args;
    time_t now;
    struct tm *t;
    char  *p;
    char   errstr[MAX_ERROR_STR];
    char   str[MAX_INFO_STR];

    if (logs->fd < 0) {
        printf("\ninvalid file fd\n");
        return;
    }

    if (logs->log_level < level)
        return;

    if (level >= DEBUG_LAST)
        return;

    errstr[0] = '\n';
    errstr[1] = '\r';
    
    now = time(NULL);

    t = localtime(&now);
    strftime(str,100,"%Y-%m-%d %H:%M:%S ",t); 
	
    memcpy(errstr+count+2, str, strlen(str) > MAX_INFO_STR ? MAX_INFO_STR : strlen(str));
    count += strlen(str);
    errstr[++count] = ' ';
    
    p = errstr + (++count);
	
    sprintf(p, "[%s] file:%s fun:%s line:%d ", log_table[level], file,fun, line);
    count += strlen(p);
    errstr[count] = ' ';
    errstr[++count] = ' ';

    p = errstr + (++count);
	
    va_start(args, fmt);
    vsprintf(p, fmt, args);
    va_end(args);

    count += strlen(p);
    errstr[++count] = '\n';

    mutex_lock();
    
    fwrite(errstr, 1, count,logs->fp);
    log->size += count;
    
    if (log->size > MAX_LOG) {
        if (!log->std) {
            char filename[MAX_NAME];
            int level;
            level = logs->log_level;
            memcpy(filename, log->file, strlen(logs->file)+1);
            unlink(logs->file);
            close_log();
            init_log(filename, level);
        }
    }
    mutex_unlock();

    if (level < DEBUG_LAST)//DEBUG_LAST
        fflush(logs->fp);
    return;
}
