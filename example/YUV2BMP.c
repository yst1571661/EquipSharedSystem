#include "YUV2BMP.h"
#include <stdio.h>

long LimitVal(long srcVal, long minVal, long maxVal)
{
	if(srcVal > maxVal)
		return  maxVal;
	if(srcVal < minVal)
		return minVal;
	return srcVal;
	
}


int YUV2RGB(unsigned char Y, unsigned char U, unsigned char V, MyRGB *pDest)
{
	if(NULL == pDest)
		return -1;
		
	long tmpY,tmpU, tmpV;
	tmpY = Y;
	tmpU = U;
	tmpV = V;
	
	long tmpR, tmpG, tmpB;
	
	tmpR = (298*(tmpY - 16) + 409 * (tmpV -128) + 128)>>8;
	tmpG = (298*(tmpY - 16) - 100 * (tmpU- 128) - 208 * (tmpV - 128) + 128) >> 8;
	tmpB = (298*(tmpY - 16) + 516*(tmpU - 128) + 128) >> 8; 
	
	pDest->r = LimitVal(tmpR, 0, 255);
	pDest->g = LimitVal(tmpG, 0, 255);
	pDest->b = LimitVal(tmpB, 0, 255);
	/*
	long rdif, invgdif, bdif;
	
	long tmpY,tmpU, tmpV;
	tmpY = Y;
	tmpU = U;
	tmpV = V;
	
	tmpU -= 128;
	tmpV -= 128;
	rdif = tmpV + ((tmpV*103) >> 8);
	invgdif = ((tmpU * 88) >> 8) + ((tmpV *183) >> 8);
	bdif = tmpU + ((tmpU * 198) >> 8);
	
	pDest->r = LimitVal(tmpY + rdif, 0, 255);
	pDest->g = LimitVal(tmpY - invgdif, 0, 255);
	pDest->b = LimitVal(tmpY + bdif, 0, 255);
	*/
	
	/*
	long tmpY,tmpU, tmpV;
	long tmpR, tmpG, tmpB;
	tmpY = Y;
	tmpU = U;
	tmpV = V;
	tmpU -= 128;
	tmpV -= 128;
	
	tmpR = tmpY + 1.402f * tmpV;
	tmpG = tmpY - 0.34414f * tmpU - 0.71414f * tmpV;
	tmpB = tmpY + 1.772f * tmpU;
	
	pDest->r = LimitVal(tmpR, 0, 255);
	pDest->g = LimitVal(tmpG, 0, 255);
	pDest->b = LimitVal(tmpB, 0, 255);
	*/
	
	
	return 0;
}


int YUV2BMP(unsigned char *pSrcBuffer, long width, long height, const char *fileName)
{
	int i = 0;
	int j = 0;
	MyRGB tmpRGB;
	long size = width * height;
	long index;
	FILE *pf;

	if(NULL == pSrcBuffer)
		return -1;
		
	pf = fopen(fileName, "w+");
	if(NULL == pf)
		return -1;
		
	if(0 != WriteBmpFileHead(pf, width, height))
		return -1;
		
	for(i = height - 1; i > -1; i--)
	{
		for(j = 0; j < width; j++)
		{
			index =  i * width + j;
			YUV2RGB(pSrcBuffer[index], pSrcBuffer[index / 4 + size], pSrcBuffer[index / 4 + size * 5 / 4] , &tmpRGB);
			fwrite(&tmpRGB, 3, 1, pf);
		}
	
	}
	
	fclose(pf);
	
	return 0;


}

int WriteBmpFileHead(FILE *pf, long width, long height)
{
	char head[54];
	long tmpLong;
	memset(head, 0, 54);
	
	if(NULL == pf)
		return -1;
	//位图文件头
	//文件类型BMP
	head[0] = 'B';
	head[1] = 'M';
	
	//总字节数
	tmpLong = width * height * 3 + 54;
	memcpy(head + 2, &tmpLong, 4);
	
	//54字节开始为位图信息
	tmpLong = 54;
	memcpy(head + 10, &tmpLong, 4);
	
	//位图信息头长度40
	tmpLong = 40;
	memcpy(head + 14, &tmpLong, 4);
	
	//位图宽度
	tmpLong = width;
	memcpy(head + 18, &tmpLong, 4);
	
	//位图高度
	tmpLong = height;
	memcpy(head + 22, &tmpLong, 4);
	
	head[26] = 1;
	//24位真彩色
	head[28] = 24;
	
	if(1 != fwrite(head, 54, 1, pf))
		return -1;
	
	return 0;
	
	
}
