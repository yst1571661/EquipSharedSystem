/*
 * =====================================================================================
 *       Copyright (C), 2010-2020, Bridge Team
 *       Filename:  spct_fileopt.h
 *       Compiler:  gcc
 *
 *    Description:  define file operate
 *         Others:  none
 *        History:
 *
 *        Version:  1.0
 *        Created:  07/07/10 12:32:17
 *         Author:  liukai , liukai@sunnorth.com.cn
 *        Company:  Sunplus Core Technology
 *
 *   Modification:
 *
 *
 * =====================================================================================
 */



#ifndef  SPCT_FILEOPT_INC
#define  SPCT_FILEOPT_INC


#include "spct_type.h"

#define		CODEC_NULL		0
#define		CODEC_H264		1
#define		CODEC_MS_G723	2
#define		CODEC_CT_G723	3
#define		CODEC_MP3		4
#define		CODEC_PCM_LE16	5
#define		CODEC_G723		6
#define		CODEC_MS_MPEG4	7
#define CODEC_INTEL_G723 9

typedef struct file_property
{
	int audio_codec;
	int video_codec;
	int width;
	int height;
	int audioen;
	int videoen;
	int channels;
	int sample_rate;
	int bit_rate;
} File_Property;

typedef struct file_opt
{
	int (* create)(struct file_opt * file, char * name, File_Property * file_property);
	int (* write)(struct file_opt * file_opt, Spct_Data * data);
	int (* close)(struct file_opt * file_opt);
	void * prev;
} File_Opt;

/***************************************************************************
Function:  fileopt_construct
Description: consruct file operation object
Input:	    	file_opt, the struct of file operation
Output:		null
Return:		0 success, others return error code
Others:		null
 ***************************************************************************/

int fileopt_construct(File_Opt * file_opt);

/***************************************************************************
Function:  fileopt_destruct
Description: deconstruct file operation object
Input:	  	file_opt, the struct of file operation
Output:		null
Return:		0 success, others return error code
Others:		null
 ***************************************************************************/

int fileopt_destruct(File_Opt * file_opt);

/***************************************************************************
Function:  fileopt_create
Description: 	create the file , the file property define the file propety, and the file name.
			open file and construct file header, and write file header to file
Input:		file_opt, the struct of file operation
			filename, file name
			file_proty, the file proerty
Output:		null
Return:		0 success, others return error code
Others:		null
 ***************************************************************************/

int fileopt_create(File_Opt * file_opt, char * filename,  File_Property * file_proty);


/***************************************************************************
Function:  fileopt_write_data
Description: write data to file
Input:		file_opt, the struct of file operation
			data, the data
Output:		null
Return:		0 success, others return error code
Others:		null
***************************************************************************/

int fileopt_write_data(File_Opt * file_opt, Spct_Data * data);

/***************************************************************************
Function:  fileopt_close
Description:    	close the file
Input:		file_opt, the struct of file operation
Output:		null
Return:		0 success, others return error code
Others:		null
 ***************************************************************************/
int fileopt_close(File_Opt * file_opt);

#endif   /* ----- #ifndef SPCT_FILEOPT_INC  ----- */

