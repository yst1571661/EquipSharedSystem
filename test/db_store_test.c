#include <stdio.h>
#include <string.h>
#include "db.h"

int main(int argc,char **argv)
{
	static GDBM_FILE gdbm_stotest = NULL;
	datum key,data;
	int i;
	for(i = 1;i<argc;i++)
	{
		key.dptr = "store test1!";
		key.dsize = strlen("store test1")+1;
		data = key;
		gdbm_stotest = db_open(argv[i]);
		printf("\n--------open dbm id:%d-----------\n",gdbm_stotest);
		if(db_store(gdbm_stotest,key,data) < 0) {
			printf("\n---------store err-----------\n");
			db_close(gdbm_stotest);
			break;
		}
		else {
			printf("\n---------store successfully!-----------\n");
			db_close(gdbm_stotest);
		}
		printf("\n--------close dbm id:%d-----------\n",gdbm_stotest);
		key.dptr = "store test2!";
		key.dsize = strlen("store test1")+1;
		data = key;
		gdbm_stotest = db_open(argv[i]);
		printf("\n--------open dbm id:%d-----------\n",gdbm_stotest);
		if(db_store(gdbm_stotest,key,data) < 0) {
			printf("\n---------store err-----------\n");
			db_close(gdbm_stotest);
			break;
		}
		else {
			printf("\n---------store successfully!-----------\n");
			db_close(gdbm_stotest);
		}
		printf("\n--------close dbm id:%d-----------\n",gdbm_stotest);
		
		gdbm_stotest = db_open(argv[i]);
		key.dptr = "store test1!";
		key.dsize = strlen("store test1!")+1;

		data = key;
		if(gdbm_exists(gdbm_stotest,key) != 0){
			printf("-------really exist here!-----");
		}
		else {
			printf("-------not exist here!-------");		
		}
	}
}
