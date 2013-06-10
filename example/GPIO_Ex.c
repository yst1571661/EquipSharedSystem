#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
//#include "nuc900_gpio.h"
#include <linux/ioctl.h>

typedef unsigned int     uint32; 
#define GPIO_IOC_MAGIC      0xd0

#define GPIO_SET_PIN            _IO(GPIO_IOC_MAGIC,    0)       /* let pin high */
#define GPIO_CLR_PIN            _IO(GPIO_IOC_MAGIC,    2)       /* let pin low   */
#define GPIO_READ_PIN           _IOWR(GPIO_IOC_MAGIC, 8, uint32)  /* read pin's set */



static int gpio_fd = -1;
extern unsigned char BCD_decode_tab[];

/*!
 * \brief init gpio
 * \return -1:fail 0:sucess
 */
int init_gpio_e()
{
	gpio_fd = open("/dev/GPIO" , O_RDWR | O_NONBLOCK);
	if(-1 == gpio_fd)
	{
		//Ch450Write(BCD_decode_tab[3],BCD_decode_tab[0],BCD_decode_tab[0]);
		printf("GPIO_EX:can not init GPIOE\n");
		return -1;
	}
	else
	{
		//Ch450Write(BCD_decode_tab[0],BCD_decode_tab[0],BCD_decode_tab[0]);
	}
	return 0;
}

void close_gpio_e()
{
	if(-1 == gpio_fd)
		close(gpio_fd);
	gpio_fd = -1;
}

/*!
 * \brief read voltage of gpioe[7]
 * \return 1:3.3v 0:0V -1:read failed
 */
int ReadVol()
{
	int arg = 7;
	if(-1 == gpio_fd)
	{
		printf("GPIO_EX: GPIOE is not init \n");
		return -1;
	}
	
	ioctl(gpio_fd, GPIO_READ_PIN, &arg);
	
	return arg;
}

/*!
 * \brief turn led on or of from gpioe[6]
 * \param [in] val if val == 1 turn led on;if val == 0, turn led off 
 * \return 0:success -1:false
 */
int SetLed(int val)
{
	const int arg = 6;
	if(-1 == gpio_fd)
	{
		printf("GPIO_EX: GPIOE is not init \n");
		return -1;
	}
	if(1 == val)
		ioctl(gpio_fd, GPIO_SET_PIN, arg);
	else if(0 == val)
		ioctl(gpio_fd, GPIO_CLR_PIN, arg);
	else
		return -1;
	return 0;
}

/*!
 * \brief turn led on 
 * \return 0:success -1:false
 */
int TurnLedOn()
{
	return SetLed(1);
}

/*!
 * \brief turn led off 
 * \return 0:success -1:false
 */
int TurnLedOff()
{
	return SetLed(0);
}
