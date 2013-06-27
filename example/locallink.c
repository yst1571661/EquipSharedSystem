#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/**********************************************************************
* �������ƣ� GetNetStat
* ���������� ������������Ƿ�Ͽ�
* ��������� 
* ��������� ��
* �� �� ֵ�� ��������1,�Ͽ�����-1
* ����˵���� ��������Ҫ�����û�Ȩ�޲��ܳɹ�����ifconfig����
* �޸�����        �汾��     �޸���          �޸�����
* ---------------------------------------------------------------------
* 2010/04/02      V1.0      eden_mgqw
***********************************************************************/ 
extern unsigned char BCD_decode_tab[];
extern int ledtwinklebegin;
extern int sockreleasebegin;
int GetNetStat( )
{
    char    buffer[BUFSIZ];
    FILE    *read_fp;
    int        chars_read;
    int        ret;
    
    memset( buffer, 0, BUFSIZ );
    read_fp = popen("ifconfig eth0 | grep RUNNING", "r");
    if ( read_fp != NULL ) 
    {
        chars_read = fread(buffer, sizeof(char), BUFSIZ-1, read_fp);
        if (chars_read > 0) 
        {
            ret = 1;
			//Ch450Write(BCD_decode_tab[0],BCD_decode_tab[0],BCD_decode_tab[0]);
			ledtwinklebegin = 0;
			sockreleasebegin = 0;
        }
        else
        {
            ret = -1;
			//Ch450Write(BCD_decode_tab[0],BCD_decode_tab[0],BCD_decode_tab[1]);
			ledtwinklebegin = 1;
			sockreleasebegin = 1;
        }
        pclose(read_fp);
    }
    else
    {
        ret = -1;
		//Ch450Write(BCD_decode_tab[0],BCD_decode_tab[0],BCD_decode_tab[1]);
		ledtwinklebegin = 1;
		sockreleasebegin = 1;
    }

    return ret;
}


/*int main()
{
    int i=0;
    i = GetNetStat();
    printf( "\nNetStat = %d\n", i );
    return 0;
}*/
