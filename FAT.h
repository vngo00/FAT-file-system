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

extern int * fat_array; // cached FAT in mem
extern int blocks_per_fat;

//Function to initiate the fat
int fat_init(uint64_t number_of_blocks, uint64_t block_size);

//function to load the fat from disk
int fat_read_from_disk();

//functino to allocate blocks needed for fat
int allocate_blocks(int blocks);

//functin to free blocks from fat
uint32_t release_blocks(int start_block);

//function to allocate more blocks if needed
void allocate_additional_blocks(uint32_t start_block, uint32_t blocks);

//function to get next block from fat
uint32_t get_next_block(int current_block);


#endif //__FAT_H__


