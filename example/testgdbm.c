#include "gdbm.h"
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
int main(int argc , char **argv)
{
	GDBM_FILE file;
	datum key;
	datum data;
	int count = 0;
	file = gdbm_open(argv[1], 0,  GDBM_WRCREAT | GDBM_SYNC, S_IRUSR | S_IWUSR, NULL);
	if (file == NULL)
		printf("\nerr\n");
	key = gdbm_firstkey(file);
	
	while (key.dptr != NULL) {
		count++;
		printf("\ncount = %d\n", count);
		printf("%s\n", key.dptr);
		data = key;
		key = gdbm_nextkey(file, key);
		free(data.dptr);
	}	
	
	
	return 0;
}
