#include <stdio.h>
#include "db.h"

int main(int argc,char **argv)
{
	static GDBM_FILE gdbm_user;
	datum data,key;
	char card_char[10] = {0};
	int card_int = 12345678,i,count=0;
	
/*	if(argc == 1){
		gdbm_user = db_open("user1.xml");
		
		for(i=0;i<9;i++){
			sprintf(card_char,"%08d",card_int++);
			key.dptr = card_char;
			key.dsize = sizeof(card_char);
			if(db_store(gdbm_user,key,data)<0){
				printf("\n--------there is no user.xml,open err-------\n");
			}
		}
	}
	else if(argc == 1) {
*/
	for (i=1;i<argc;i++)
	{
		gdbm_user = db_open(argv[i]);
		key = gdbm_firstkey(gdbm_user);
		while(key.dptr) {
			count++;
			printf("\n-----------count = %d-------------\n",count);
			if(gdbm_user == NULL)
				printf("\n---------err---------\n");
			else {
				data = gdbm_fetch(gdbm_user,key);
				printf("\n----------%s------------\n",key.dptr);
				if(data.dptr == NULL){
					db_close(gdbm_user);
				}
				else{
				printf("\n----------%s------------\n",data.dptr);
				}
			}
		key = gdbm_nextkey(gdbm_user,key);
		}
	}
}
