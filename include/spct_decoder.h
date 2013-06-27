/*
 * =====================================================================================
 *       Copyright (C), 2010-2020, Bridge Team
 *       Filename:  spct_decode.h
 *       Compiler:  gcc
 *
 *    Description:  define spct decoder g723 interface
 *         Others:  none
 *        History:
 *
 *        Version:  1.0
 *        Created:  09/15/10 12:25:48
 *         Author:  liukai , liukai@sunnorth.com.cn
 *        Company:  Sunplus Core Technology
 *
 *   Modification:
 *
 *
 * =====================================================================================
 */



#ifndef  SPCT_DECODE_INC
#define  SPCT_DECODE_INC

#define SPCT_CODEC_H264 0x80000
#define SPCT_CODEC_G723 0x80010

typedef struct {
	int codec_id;
	void * prev;
} Decode_Context;


/***************************************************************************
Function:  spct_decode_open
Description:open decoder, this function will initialize the decoder context 
Input:		codec_id, which codec 
			context, the context of decoder
			needparser, 0 not parser, 1 need parser
Output:		null
Return:		0 success, -1 fail
			the needparser paramera, is used by h264 codec.
			when the source that you pass to decoder , is bit stream readed from file or others,
		   	you may need decoder create parser automicly, the parser will parse the bitstream and divided it to one frame that decoder can process.
			but if the source is get from spct streaming client API, the data is a completely frame,
			you do not need decoder to create parser(must needparser = 0), if needparser = 1 , decoder would not decode correctly. 
Others:		null
 ***************************************************************************/

int spct_decode_open(int codec_id, Decode_Context * context, int needparser);


/***************************************************************************
Function:  spct_decode_close
Description:close decoder context, this function will free the resource whick decoder context alloc
Input:      context, the decoder context
Output:		null
Return:		0 success, -1 fail
Others:		null
 ***************************************************************************/

void spct_decode_close(Decode_Context * context);


/***************************************************************************
Function:  spct_decode
Description:decode the src_buf to dest_buf
Input:		context, the decoder context
			src_buf, the compressed bit stream data
			src_len, the bit stream length
			dest_buf, the output data buffer,,
			dest_len, the output data size
			len,  the len decoder put in dest_buf
Output:		dest_buf, the ouput data will put in this buffer
			len, the output data length
Return:		-1 decode error, others meen the processed data size, maybe equal src_len or less then src_len 
			if len == 0 and return value != -1, meen that decoder need more data input, continue call this function again
			if return value == -1 error occur
			if return value != -1 and len != 0, meen that one frame decodesuccess, you can use it , the content is point by dest_buf
Others:		null
 ***************************************************************************/

int spct_decode(Decode_Context * context, unsigned char * src_buf, int src_len,
		unsigned char ** dest_buf, int * len);


#endif   /* ----- #ifndef SPCT_DECODE_G723_INC  ----- */

