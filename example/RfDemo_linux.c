#include <stdio.h>
#include <termio.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include "librf.h"

static int icdev = -1;
extern unsigned char BCD_decode_tab[];
extern int ledtwinklebegin;
int         init_card_uart()
{
        int i,st;
        int par=1;
        long baud=115200;
        static unsigned char buff[16];
        static unsigned char data[16];
        unsigned char tk[6]={
        0xff,0xff,0xff,0xff,0xff,0xff
        };
        for(i=0;i<16;i++)
        {
          buff[i]=0xff;
          data[i]=0x33;
        }
        icdev=dc_init(par,baud);
        if(icdev<0)
        {
          printf("\nrf_init error.");
          //Ch450Write(BCD_decode_tab[0],BCD_decode_tab[0],BCD_decode_tab[2]);
          ledtwinklebegin = 1;
          return 0;
        }
        else
        {
                //Ch450Write(BCD_decode_tab[0],BCD_decode_tab[0],BCD_decode_tab[0]);
                ledtwinklebegin = 0;
        }

        if(dc_load_key(icdev,0,0,tk)!=0)
        {
          printf("\nload_key (A) error.\n");
          dc_exit(icdev);
          return 0;
        }
        else
        {
                //Ch450Write(BCD_decode_tab[0],BCD_decode_tab[0],BCD_decode_tab[0]);
                ledtwinklebegin = 0;
        }
         printf("\n----- init card_uart ok-----\n");

}

int CardRead()
{
    unsigned int tagtype;
    unsigned long cardsnr = 0;
    unsigned char strSize[5];
    if((dc_reset(icdev,0))!=0) {printf("\ndc_reset error!"); return -2;}

    cardsnr=0;
     // if((dc_card(icdev,0,&snr))!=0) {printf("dc_card error!\n"); continue;}

    if( dc_request(icdev,0,&tagtype)!=0) {/*printf("dc_request error!\n")*/; return 0;}

    if(dc_anticoll(icdev,0,&cardsnr)!=0){printf("\ndc_anticoll error!"); return -2;}

    if(dc_select(icdev,cardsnr,strSize)!=0){printf("\ndc_select error!"); return -2;}


    //printf("serial number: %08lX\n",cardsnr);

    //dc_exit(icdev);
    return cardsnr;
}

/*
unsigned long CardRead()
{
    //int i,st;
    static unsigned long snr;
    //static unsigned char data[16];
    //static unsigned char buff[16];
    //int par=1;
    //long baud=115200;
    //int icdev;
    unsigned long rdata;
    unsigned int tagtype;
    unsigned long cardsnr = 0;
    unsigned char strSize[5];


    if((dc_reset(icdev,0))!=0) {printf("dc_reset error!\n"); return 1;}

    cardsnr=0;
    // if((dc_card(icdev,0,&snr))!=0) {printf("dc_card error!\n"); continue;}

    if( dc_request(icdev,0,&tagtype)!=0) {//printf("dc_request error!\n"); return 0;}

    if(dc_anticoll(icdev,0,&cardsnr)!=0){printf("dc_anticoll error!\n"); return 1;}

    if(dc_select(icdev,cardsnr,strSize)!=0){printf("dc_select error!\n"); return 1;}


    //printf("serial number: %08lX\n",cardsnr);

    //  dc_exit(icdev);
    return cardsnr;
}
*/
void close_card_uart()
{
        dc_exit(icdev);
}

void card_beep(int msec)
{
        int st;
        if(st=(dc_halt(icdev))==0)
        {
                st=dc_beep(icdev,msec);
        }
        else
        {
            printf("\ndc_halt 0x%02x",st);
        }
}
