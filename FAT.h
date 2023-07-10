/**************************************************************
* Class:  CSC-415-01 Summer 2023
* Names: Tyler Fulinara, Rafael Sant Ana Leitao, Anthony Silva , Vinh Ngo Rafael Fabiani
* Student IDs: 922002234, 920984945,
922907645, 921919541,
922965105
* GitHub Name: rf922
* Group Name: MKFS
* Project: Basic File System
*
* File: FAT.h
*
* Description: free space management
**************************************************************/
#ifndef __FAT_H__
#define __FAT_H__

#include <stdint.h>
#define FAT_BLOCK_START 1
#define RESERVED_BLOCKS 154 // assuming FAT will have 19531 blocks, 154 will be reserved for VCB and FAT itself

int init_FAT(uint64_t num_blocks, uint64_t block_size);
int read_FAT_from_disk(void);
int allocate_blocks(void);
uint32_t release_blocks(int start_block);
int allocate_additional_block(uint32_t start_block);
uint32_t get_next_block(int current_block);


#endif //__FAT_H__


