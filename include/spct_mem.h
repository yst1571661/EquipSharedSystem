/*
 * =====================================================================================
 *       Copyright (C), 2010-2020, Bridge Team
 *       Filename:  spct_mem.h
 *       Compiler:  gcc
 *
 *    Description:  this file define memory block
 *         Others:  none
 *        History:
 *
 *        Version:  1.0
 *        Created:  04/14/10 11:27:28
 *         Author:  wangheng, heng.wang@sunnorth.com.cn
 *        Company:  Sunplus Core Technology
 *
 *   Modification:
 *
 *
 * =====================================================================================
 */

#ifndef _SPCT_MEM_H_
#define _SPCT_MEM_H_

#include <unistd.h>
#include <pthread.h>

typedef struct mem_block {
	struct mem_block * next;
	char   data[1];
} Mem_Block;

typedef Mem_Block Mem_Node;

typedef struct mem {
	int node_size;
	int node_num;
	int block_size;
	char * free;
	char * end;
	Mem_Node * free_node;
	Mem_Block * header;
	Mem_Block * tail;
	Mem_Block * cur;
} Mem;

/***************************************************************************
Function:		mem_construct
Description:	construct a object Mem to manage memory, the fuction mem_alloc
				will allocate block_size * block_num bytes at the first time, 
				and return the address direct every time until the block_num blocks 
				used up. Then allocate again.
Input:			block_size, the block size
				block_num, the number of blocks in group
Output:			mem, object Men
Return:			Success: SUCCESS, Fail: -1
Others:			not thread save
 ***************************************************************************/
int mem_construct (Mem * mem, int block_size, int block_num);


/***************************************************************************
Function:		mem_destruct
Description:	destruct the memory manage object
Input:			mem, Mem object
Output:			NULL
Return:			NULL
Others:			not thread save
 ***************************************************************************/
void mem_destruct (Mem * mem);


/***************************************************************************
Function:		mem_alloc
Description:	allocates block_size bytes and returns a pointer to the allocated memory.
				The block_size is user constructed. The memory is not cleared.
Input:			mem, Mem object
Output:			NULL
Return:			NULL
Others:			not thread save
 ***************************************************************************/
void * mem_alloc (Mem * mem);


/***************************************************************************
Function:		mem_free
Description:	frees the memory space pointed to by addr, 
				Free a block_size memory space
Input:			mem, Mem object
				addr, the address will be free
Output:			NULL
Return:			NULL
Others:			not thread save
 ***************************************************************************/
void mem_free (Mem * mem, void * addr);




/***************************************************************************
Function:  mem_reset
Description:reset memory, clear all object allocate in this memory 
Input:		mem, mem obj
Output:		null
Return:		0 success, others return error code
Others:		null
 ***************************************************************************/

void mem_reset (Mem * mem);


#endif

