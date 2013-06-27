#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/**********************************************************************
* 函数名称： GetNetStat
* 功能描述： 检测网络链接是否断开
* 输入参数： 
* 输出参数： 无
* 返 回 值： 正常链接1,断开返回-1
* 其它说明： 本程序需要超级用户权限才能成功调用ifconfig命令
* 修改日期        版本号     修改人          修改内容
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
