/****************************************************************************

 *                                                                          *

 * Copyright (c) 2004 - 2006 Winbond Electronics Corp. All rights reserved. *

 *                                                                          *

 ****************************************************************************/ 

/*********************************************************************************

 * 

 * FILENAME

 *     rtc.h

 *

 * VERSION

 *     1.0

 *

 * DESCRIPTION

 *     Library for RTC

 *

 * DATA STRUCTURES

 *     None

 *

 * FUNCTIONS

 *     None

 *

 * HISTORY

 *     09/05/2005            Ver 1.0 Created by PC34 YHan

 *

 * REMARK

 *     None

 **************************************************************************/


#ifndef _RTC_H
#define _RTC_H


#define HR24 1
#define HR12 0
#define RTC_TIME_SCALE	0x10

#define SIGRTC 26
#define ALARM_IRQ 1
#define TICK_IRQ 2

/* Device DS3231 Slave address */
#define DS3231_ADR           0x68

/* DS3231 Time registers address*/
#define DS3231_REG_SEC        0x00    //��Ĵ�����ַ
#define DS3231_REG_MIN        0x01    //�ּĴ�����ַ
#define DS3231_REG_HOUR       0x02    //Сʱ�Ĵ�����ַ
#define DS3231_REG_WEEK       0x03    //���ڼĴ�����ַ
#define DS3231_REG_DATE       0x04    //���ڼĴ�����ַ
#define DS3231_REG_MONTH      0x05    //�¼Ĵ�����ַ
#define DS3231_REG_YEAR       0x06    //��Ĵ�����ַ

/* DS3231 Special Funtion registers address*/
#define DS3231_REG_CONTROL    0x0e      //���ƴ�����ַ
#define DS3231_REG_STATUS     0x0f      //״̬������ַ
#define DS3231_REG_COMPENSATE 0x10      //����������ַ 
#define DS3231_REG_TEM_MSB    0x11      //�¶ȼĴ�����ַ(���ֽ�)
#define DS3231_REG_TEM_LSB    0x12      //�¶ȴ�����ַ(���ֽ�)



static int fd_i2c;
static int fd_i2c0;
static int irq;		//alarm or tick

struct rtc_time {
	unsigned int tm_sec;
	unsigned int tm_min;
	unsigned int tm_hour;
	unsigned int tm_mday;
	unsigned int tm_mon;
	unsigned int tm_year;
	unsigned int tm_wday;
	unsigned int tm_yday;
  	unsigned int tm_isdst;
};

/*
eeprom  table
addr      fun            value
0         web_en         11 
1 - 11    web_par
30        ip_en          11
31 - 34   ipdz
35 - 38   zwym
39 - 42   mywg
45        led_en         11
46        led_on
246 - 248 eeprom_check   1 2 3 
*/

#endif
