#include <fcntl.h>
#include <linux/sysctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include "rtc.h"
#include "bcd.h"
#include "tcp_connect.h"
#include "log.h"

#if  	DEBUG_LOG
#define DebugPrintf(args...)	log_error(LOG_DEBUG,##args)
#else
#define DebugPrintf	printf
#endif


struct rtc_time rtc_tm;

int init_ds3231()
{
        unsigned char i2cdata[3];
        int ret;

        fd_i2c = open("/dev/i2c1", O_RDWR);
        if (fd_i2c == -1)
        {
                DebugPrintf("\nCan't open /dev/i2c1!");
                return -1;
        }
        else
        {
                ret = ioctl(fd_i2c, 0x0703, DS3231_ADR);	//首先设置DS3231的地址
                DebugPrintf("\nSet ds3231 address OK! ");

                i2cdata[0] = DS3231_REG_CONTROL;			//其次设置DS3231的控制寄存器
                i2cdata[1] = 0x04;               			//00000100:review ds3231.pdf for more detail
                if(write(fd_i2c, &i2cdata[0], 2) != 2)
                DebugPrintf("\ninit ds3231 error !");
                else
                 DebugPrintf("\ninit ds3231 OK!");
                return 1;
        }
}

void get_time(struct rtc_time *tm)
{
  int i;
  char rtc_string[40];
  unsigned char buf[7],temp;
  unsigned char i2cdata[3];
  if(fd_i2c)
  {
    for(i=0;i<7;i++)
    {
     i2cdata[0] = i;
     if(write(fd_i2c, &i2cdata[0], 1) != 1)		//first:write time address
                DebugPrintf("\nwrite error at %d\n", i);
     if (read(fd_i2c, &i2cdata[0], 1) != 1) 	//second:read data
            DebugPrintf("\nread error at %d\n", i);
     else
        buf[i] = i2cdata[0];
    }
    tm->tm_sec = BCD2BIN(buf[0]);
    tm->tm_min = BCD2BIN(buf[1]);
    tm->tm_hour = BCD2BIN(buf[2]);
    tm->tm_wday = BCD2BIN(buf[3]);				//这是星期,实际应用中不用
    tm->tm_mday = BCD2BIN(buf[4]);

    buf[5] == buf[5]&&0x7f;   //don't care bit 7 in month register
    tm->tm_mon = BCD2BIN(buf[5]);
    tm->tm_year = BCD2BIN(buf[6])+2000;

        DebugPrintf("\n-----read rtc %02d%02d%02d%02d%d.%02d-----\n",tm->tm_mon,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_year,tm->tm_sec);
         sprintf(rtc_string,"date %02d%02d%02d%02d%d.%02d",tm->tm_mon,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_year,tm->tm_sec);
         system(rtc_string);
  }

  return ;

}

void getstime(struct tm *tm)
{
  int i;
  char rtc_string[40];
  unsigned char buf[7],temp;
  unsigned char i2cdata[3];
  if(fd_i2c)
  {
    for(i=0;i<7;i++)
    {
     i2cdata[0] = i;
     if(write(fd_i2c, &i2cdata[0], 1) != 1)		//first:write time address
        DebugPrintf("\nwrite error at %d", i);
     if (read(fd_i2c, &i2cdata[0], 1) != 1) 	//second:read data
        DebugPrintf("\nread error at %d", i);
     else
        buf[i] = i2cdata[0];
    }
    tm->tm_sec = BCD2BIN(buf[0]);
    tm->tm_min = BCD2BIN(buf[1]);
    tm->tm_hour = BCD2BIN(buf[2]);
    tm->tm_wday = BCD2BIN(buf[3]);				//这是星期,实际应用中不用
    tm->tm_mday = BCD2BIN(buf[4]);

    buf[5] == buf[5]&&0x7f;   //don't care bit 7 in month register
    tm->tm_mon = BCD2BIN(buf[5]);
    tm->tm_year = BCD2BIN(buf[6])+2000;

  //  DebugPrintf("\n-----read rtc %02d%02d%02d%02d%d.%02d-----\n",tm->tm_mon,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_year,tm->tm_sec);
  //   sDebugPrintf(rtc_string,"date %02d%02d%02d%02d%d.%02d",tm->tm_mon,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_year,tm->tm_sec);
    // system(rtc_string);
  }

  return ;

}


int  set_time(unsigned int year0,unsigned int year1,unsigned int month,unsigned int day,unsigned int hour,unsigned int min,unsigned int sec)
{
        int i;
        unsigned char buf[7];
        unsigned char i2cdata[3];

        rtc_tm.tm_year =year1;
        rtc_tm.tm_mon = month;
        rtc_tm.tm_mday = day;
        rtc_tm.tm_hour = hour;
        rtc_tm.tm_min = min;
        rtc_tm.tm_sec = sec;

        DebugPrintf("\nwrite rtc %02d%02d%02d%02d%d.%02d",rtc_tm.tm_mon,rtc_tm.tm_mday,rtc_tm.tm_hour,rtc_tm.tm_min,rtc_tm.tm_year,rtc_tm.tm_sec);

        buf[0] = BIN2BCD(rtc_tm.tm_sec);
        buf[1] = BIN2BCD(rtc_tm.tm_min);
        buf[2] = BIN2BCD(rtc_tm.tm_hour);
        buf[3] = 0x01;             				//由于设置时间时不用星期这个项，故每次设置时默认为星期一
        buf[4] = BIN2BCD(rtc_tm.tm_mday);
        buf[5] = BIN2BCD(rtc_tm.tm_mon);
        buf[6] = BIN2BCD(rtc_tm.tm_year);

        if (fd_i2c)
        {
                 for(i=0;i<7;i++)
                 {
                          i2cdata[0] = i;
                          i2cdata[1] = buf[i];
                          if (write(fd_i2c,&i2cdata[0], 2) != 2)
                                {
                                        DebugPrintf("\nWrite error at %d", i);
                                        return -1;
                                }
                 }
        }
  return 0;
}

int Set_time(unsigned char * time_buf)
{
        int ret;
        char rtc_string[40];
        ret = set_time(time_buf[0],time_buf[1],time_buf[2],time_buf[3],time_buf[4],time_buf[5],time_buf[6]);
        sprintf(rtc_string,"date %02d%02d%02d%02d%02d%02d.%02d",time_buf[2],time_buf[3],time_buf[4],time_buf[5],time_buf[0],time_buf[1],time_buf[6]);
        system(rtc_string);
        return ret;
}

/*int init_ch450()
{
        unsigned char i2cdata[3];
        int ret;

        fd_i2c0 = open("/dev/i2c0", O_RDWR);
        if (fd_i2c0 == -1)
        {
                DebugPrintf("Can't open /dev/i2c0!\n");
                return -1;
        }
        else
        {
                ret = ioctl(fd_i2c0, 0x0703, 0x64);	//首先设置DS3231的地址
                DebugPrintf("Set ch450 address OK! \n");

                i2cdata[0] = 0xff;			//其次设置DS3231的控制寄存器
                //i2cdata[1] = 0x01;               			//00000100:review ds3231.pdf for more detail
                if(write(fd_i2c0, &i2cdata[0], 1) != 1)
                DebugPrintf("init ch450 error !\n");
                else
                 DebugPrintf("init ch450 OK!\n");


                //i2cdata[0] = 0x64;			//其次设置DS3231的控制寄存器
                //i2cdata[1] = 0x79;               			//00000100:review ds3231.pdf for more detail
                //if(write(fd_i2c0, &i2cdata[0], 2) != 2)
                //DebugPrintf("set ch450 error !\n");
                //else
                // DebugPrintf("set ch450 OK!\n");


                return 1;
        }
}*/

int init_at24c02b()
{
        unsigned char i2cdata[3];
        int ret;

        fd_i2c0 = open("/dev/i2c0", O_RDWR);
        if (fd_i2c0 == -1)
        {
                DebugPrintf("Can't open /dev/i2c0!\n");
                return -1;
        }
        else
        {
                ret = ioctl(fd_i2c0, 0x0703, 0x50);	//首先设置at24c02b的地址
                write_at24c02b(246,1);
                write_at24c02b(247,2);
                write_at24c02b(248,3);
                if(read_at24c02b(246)==1 && read_at24c02b(247)==2 && read_at24c02b(248)==3)
                {
                        DebugPrintf("\ninit at24c02b OK!\n");
                        return 0;
                }
                else
                {
                        DebugPrintf("\ninit at24c02b error!\n");
                        return -1;
                }

        }

}

int read_at24c02b(unsigned int addr)
{
        int buf;
        if(fd_i2c0)
        {
                buf = addr;
                if(write(fd_i2c0, &buf, 1) != 1)		//first:write time address
                {
                        DebugPrintf("write error\n");
                        return -1;
                }
                if (read(fd_i2c0, &buf, 1) != 1) 	//second:read data
                {
                        DebugPrintf("read error\n");
                        return -1;
                }
                return buf;
        }
        return -1;

}

int write_at24c02b(unsigned int addr, int data)
{
        unsigned char i2cdata[3];
        i2cdata[0] = addr;
        i2cdata[1] = data;
        if(write(fd_i2c0, &i2cdata[0], 2) != 2)
        {
                DebugPrintf("write error\n");
                return -1;
        }
        return 0;
}
