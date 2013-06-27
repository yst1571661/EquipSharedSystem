/*
 * =====================================================================================
 *       Copyright (C), 2010-2020, Bridge Team
 *       Filename:  test.c
 *       Compiler:  gcc
 *
 *    Description:  test application
 *         Others:  none
 *        History:
 *
 *        Version:  1.0
 *        Created:  09/16/10 14:02:06
 *         Author:  liukai , liukai@sunnorth.com.cn
 *        Company:  Sunplus Core Technology
 *
 *   Modification:
 *
 *
 * =====================================================================================
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "spct_decoder.h"

void usage()
{
	printf("decodertest id infile [outfile]\n");
	printf("     id, the file type, 1 is h264file, 2 g723 file\n");
	printf("     infile, the h264 raw data, or g723 raw data file\n");
	printf("     outfile, outfile name, if null, the default name is infile_out\n");
}

int main(int argc, char * argv[])
{
	char * infilename, * outfilename;
	unsigned char * outbuf;
	unsigned char * inptr;
	int outsize;
	char buf[1024];
	FILE * filein, * fileout;
	int codecid;
	int ret, len;
	Decode_Context decode_context;
	outfilename = NULL;
	if(argc < 3)
	{
		usage();
		exit(0);
	}
	ret = atoi(argv[1]);
	if( ret == 1)
		codecid = SPCT_CODEC_H264;
	else if( ret == 0)
		codecid = SPCT_CODEC_G723;
	else
	{
		printf(" ERROR codec ID\n");
		exit(1);
	}

	infilename = argv[2];
	if(argc >= 4)
	{
		outfilename = argv[3];
	}
	filein = fopen(infilename, "r");
	if(filein == NULL)
	{
		printf(" Could not open input file\n");
		exit(1);
	}
	if(outfilename == NULL)
	{
		sprintf(buf, "%s_out", infilename);
		outfilename = buf;
	}
	fileout = fopen(outfilename, "w");
	if(fileout == NULL)
	{
		printf(" Could not create output file\n");
		fclose(filein);
		exit(1);
	}
	ret = spct_decode_open(SPCT_CODEC_H264, &decode_context, 1);
	if(ret)
	{
		printf(" Open decoder error \n");
		fclose(filein);
		fclose(fileout);
		exit(1);
	}
	len = fread(buf, 1, sizeof(buf), filein);
	while(len > 0)
	{
		int readbytes;
		// decodei
		inptr = buf;
		while(len > 0)
		{
			ret = spct_decode(&decode_context, inptr, len, &outbuf, &outsize);
			if(ret != -1 && outsize != 0) // decode oneframe success save it
			{
				fwrite(outbuf, 1, outsize, fileout);
			}
			if(ret == -1)
			{
				printf("decode error\n");
				exit(1);
			}
			len -= ret;
			inptr += ret;
		}
		memmove (buf, inptr, len);
		readbytes= fread(buf + len, 1, sizeof(buf) - len, filein);
		if (readbytes <= 0) {
			break;
		}
		len += readbytes;
	}
	fclose(filein);
	fclose(fileout);
	return 0;
}
