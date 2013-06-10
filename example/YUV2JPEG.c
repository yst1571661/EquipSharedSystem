#include <stdio.h>
#include <jpeglib.h>
#include <jerror.h>
#include "YUV2JPEG.h"
#include <stdlib.h>
#include <string.h>

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

int YUV2RGBBuffer(unsigned char *pSrcBuffer, long width, long height, char *outbuffer)
{
	int i = 0;
	int j = 0;
	long k = 0;
	MyRGB tmpRGB;
	long size = width * height;
	long index;

	if(NULL == pSrcBuffer || NULL == outbuffer)
		return -1;
		
	/*for(i = 0; i < height; i++)
	{
		for(j = 0; j < width; j++)
		{
			index =  i * width + j;
			YUV2RGB(pSrcBuffer[index], pSrcBuffer[index / 4 + size], pSrcBuffer[index / 4 + size * 5 / 4] , &tmpRGB);
			memcpy(outbuffer + index * 3, &tmpRGB, 3);
		}
	
	}*/
	for(i = 0; i < height; i++)
	{
		for(j = 0; j < width; j++)
		{
			index =  i * width + j;
			k = (i / 2) * width + j;
			YUV2RGB(pSrcBuffer[index], pSrcBuffer[k / 2 + size], pSrcBuffer[k / 2 + size * 5 / 4], &tmpRGB);
			memcpy(outbuffer + index * 3, &tmpRGB, 3);
		}
	}
	return 0;
}

int convertColor(unsigned char *color, int width)
{
	int i, ptr;
	char tmp;

	for(i = 0, ptr=0; i < width; i++, ptr=i*3) 
	{
		tmp = color[ptr];
		color[ptr] = color[ptr+2];
		color[ptr+2] = tmp;
	}
}

int YUV2JPEG(unsigned char *pSrcBuffer, long width, long height, const char *outfileName)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	unsigned char *line, *buffer = NULL;
	int row_width;
	int i, j;
	FILE* out;

	if((out = fopen(outfileName,"wb")) == NULL )
	{
	  printf("BmptoJpeg: ...can't open %s\n", outfileName);
	  return -1;
	}
	cinfo.err = jpeg_std_error(&jerr);
	
	jpeg_create_compress(&cinfo);

  

	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;

	jpeg_set_defaults(&cinfo);
	jpeg_stdio_dest(&cinfo, out);
	jpeg_set_quality(&cinfo, 75, TRUE);
	jpeg_start_compress(&cinfo, TRUE);

  

  /* set row width and calulate padding data*/

  row_width = cinfo.image_width*cinfo.input_components

  + cinfo.image_width%4;

  /* memory allocate for input buffer */

  buffer = (unsigned char *)malloc(cinfo.image_width*cinfo.image_height*cinfo.input_components);

   if(NULL == buffer)
   {
	  printf("BmptoJpeg: memory allocate for input buffer failed!\n");
	  return -1;
	}
  /* input from YUV buffer */
	if(0 != YUV2RGBBuffer(pSrcBuffer, width, height, buffer))
	{
		printf("BmptoJpeg: input from YUV buffer failed!\n");
		return -1;
	}
	
	line = buffer;

  

	/* output to jpeg file */

	for (i = 0; i < cinfo.image_height; i++, line+=row_width) {

	convertColor(line, cinfo.image_width);

	jpeg_write_scanlines(&cinfo, &line, 1);

	}

  

	jpeg_finish_compress(&cinfo);

	jpeg_destroy_compress(&cinfo);

  

	fclose(out);
	free(buffer);

  

	return 0;

  }

