#ifndef DB_H_
#define DB_H_

#include "gdbm.h"

#define     RECORD_LENGTH   30
#define     DEBUG           0

#define     USERHEAD    "user_version"
#define     KEY_SIZE_MAX        20



/*******�ɹ�����0**************/

GDBM_FILE db_open(char *filename);
int db_close(GDBM_FILE ptr);

int db_store(GDBM_FILE ptr, datum key, datum value); //�洢
int db_exists(GDBM_FILE ptr, datum key);              //���ҵ�����0  ����ֵ
int db_delete(GDBM_FILE ptr, datum key);





#endif
