#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
//#include "nuc900_gpio.h"

static int myi2c_fd = -1;

 //unsigned char BCD_decode_tab[0x10] = { 0X40, 0X79, 0X24, 0X30, 0X19, 0X12, 0X02, 0X78, 0X00, 0X10, 0X08, 0X03, 0X27, 0X21, 0X06, 0X0E };
 //unsigned char BCD_decode_tab[0x10] = { 0X81, 0X9f, 0X49, 0X0d, 0X17, 0X25, 0X21, 0X8f, 0X01, 0X05, 0X03, 0X31, 0Xe1, 0X19, 0X61, 0X63 };
unsigned char BCD_decode_tab[0x10] = { 0X81, 0Xf3, 0X49, 0X61, 0X33, 0X25, 0X05, 0Xf1, 0X01, 0X21, 0X11, 0X07, 0X8d, 0X43, 0X0d, 0X1d };
 //由高到低  g f e d  c b a DP  共阳数码管
 
int init_ch450()
{
	unsigned char myi2cdata[3];
	myi2c_fd = open("/dev/MYI2C" , O_RDWR);
	if(-1 == myi2c_fd)
	{
		printf("MYI2C_EX:can not init MYI2C\n");
		return -1;
	}
	
	myi2cdata[0] = 0x64;
	myi2cdata[1] = 0X81;
	if(write(myi2c_fd, &myi2cdata[0], 2) != 0)		
		printf("init ch450 error !\n");
		
	myi2cdata[0] = 0x66;
	myi2cdata[1] = 0X81;
	if(write(myi2c_fd, &myi2cdata[0], 2) != 0)		
		printf("init ch450 error !\n");
		
	myi2cdata[0] = 0x68;
	myi2cdata[1] = 0X81;
	if(write(myi2c_fd, &myi2cdata[0], 2) != 0)		
		printf("init ch450 error !\n");
				
	myi2cdata[0] = 0x48;
	myi2cdata[1] = 0x01;
	if(write(myi2c_fd, &myi2cdata[0], 2) != 0)		
		printf("init ch450 error !\n");
	else 
		printf("init ch450 ok !\n");

}
 
/*int Ch450Write(unsigned char ch1, unsigned char ch2, unsigned char ch3)
{
	unsigned char myi2cdata[3];
	
	myi2cdata[0] = 0x64;
	myi2cdata[1] = ch1;
	if(write(myi2c_fd, &myi2cdata[0], 2) != 0)		
		printf("write ch450 error !\n");
		
	myi2cdata[0] = 0x66;
	myi2cdata[1] = ch2;
	if(write(myi2c_fd, &myi2cdata[0], 2) != 0)		
		printf("write ch450 error !\n");
		
	myi2cdata[0] = 0x68;
	myi2cdata[1] = ch3;
	if(write(myi2c_fd, &myi2cdata[0], 2) != 0)		
		printf("write ch450 error !\n");

	return 0;
}*/
