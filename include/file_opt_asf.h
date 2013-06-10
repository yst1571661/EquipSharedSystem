/*
 * =====================================================================================
 *		Copyright (C), 2010-2020, Bridge Team
 *		Filename:	file_opt_asf.h
 *		Compiler:	gcc
 *
 *		Description:	declearation of the asf operating functions
 *			 Others:	none
 *			History:	
 *			
 *			Version:	1.0
 *			Creaged:	08/02/10 15:55:08
 *			Author:		Huawei.shen Huawei.shen@sunnorth.com.cn
 *		   Company:		Sunplus Core Technology
 *
 *    Modification:
 *
 * =====================================================================================
 */
#ifndef FILE_OPT_ASF_INC
#define FILE_OPT_ASF_INC

#include "spct_fileopt.h"

/***************************************************************************
Function:	fileopt_asf_construct
Description: construct asf file operation object	
Input:			file_opt, the struct of file operation
Output:			null
Return:			0 success, others return error code
Others:			null
 ***************************************************************************/
int fileopt_asf_construct(File_Opt * file_opt);

/***************************************************************************
Function:	fileopt_asf_destruct
Description:	deconstruct file operation object
Input:		file_opt, the struct of file operation
Output:		null
Return:		0 success, others return error code
Others:		null
 ***************************************************************************/
int fileopt_asf_destruct(File_Opt * file_opt);
#endif	/* ---- #ifndef FILE_OPT_ASF_INC ---- */
