#include <stdlib.h>
#include <stdio.h>
#include "db.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <pthread.h>

int main()
{
	pthread_mutex_t card = PTHREAD_MUTEX_INITIALIZER;
	static GDBM_FILE order=NULL;
	datum key,data;
	order = db_open("1.xml");
	pthread_mutex_lock(&card);
	key.dptr = "123";
	key.dsize = 4;
	data = key;
	db_store(order,key,data);
	pthread_mutex_lock(&card);
	db_close(order);
}
