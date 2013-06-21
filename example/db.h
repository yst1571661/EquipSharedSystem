#ifndef DB_H_
#define DB_H_

#include "gdbm.h"

#define     RECORD_LENGTH   30
#define     DEBUG           0

#define     USERHEAD    "user_version"
#define     KEY_SIZE_MAX        20



/*******成功返回0**************/

GDBM_FILE db_open(char *filename);
int db_close(GDBM_FILE ptr);

int db_store(GDBM_FILE ptr, datum key, datum value); //存储
int db_exists(GDBM_FILE ptr, datum key);              //查找到返回0  否则负值
int db_delete(GDBM_FILE ptr, datum key);





#endif
