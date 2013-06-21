#include "db.h"
#include <stdlib.h>
#include <string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>

GDBM_FILE db_open(char *filename)
{
    GDBM_FILE pf;
#if DEBUG
    printf("\ndb_open");
#endif
    if (filename == NULL)
        return NULL;
    if ((pf = gdbm_open(filename, 0,  GDBM_WRCREAT | GDBM_SYNC, S_IRUSR | S_IWUSR, NULL)) != NULL)
           return pf;
    else
        return NULL;
}

int db_close(GDBM_FILE gdb_ptr)
{
#if DEBUG
    printf("\ndb_close");
#endif
    if (gdb_ptr == NULL)
        return -1;
    else
        gdbm_close(gdb_ptr);
    return 0;
}


int db_store(GDBM_FILE db_ptr, datum key, datum value)
{
#if DEBUG
    printf("\ndb_store");
#endif
    if (db_ptr == NULL)
        return -1;
    if (gdbm_store(db_ptr, key, value, GDBM_REPLACE ) != 0)
        return -1;
    return 0;
}

int db_store_nreplace(GDBM_FILE db_ptr, datum key, datum value)
{
#if DEBUG
    printf("\ndb_store_nreplace");
#endif
    if (db_ptr == NULL)
        return -1;
    if (gdbm_store(db_ptr, key, value, GDBM_INSERT ) != 0)
        return -1;
    return 0;
}



int db_exists(GDBM_FILE db_ptr, datum key)
{
#if DEBUG
    printf("\ndb_exists");
#endif
    int tmp = -10;
    tmp = gdbm_exists(db_ptr, key);
    if (tmp == 0)
        return -1;
    return 0;
}

int db_delete(GDBM_FILE db_ptr, datum key)
{
#if DEBUG
    printf("\ndb_delete");
#endif
    if (gdbm_delete(db_ptr, key) < 0)
        return -1;
    else
        return 0;
}




