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

#define FAT_BLOCK_START_LOCATION 1

extern int * fat_array; // keep a copy of FAT while program is running
extern int blocks_per_fat;

// identify the first empty block in the FAT.
// go through each block till infd a block not in use.
int find_first_empty_block_in_fat();

// upfates the FAT on disk
void update_fat_on_disk();


// initialze the FAT, calculates the size of FAT in bytes and blocks.
// set up initial FAT structure after memeory allocation for FAT.
// if memory allocation fails, an error message get logged and -1 returned.
int fat_init(uint64_t number_of_blocks, uint64_t block_size);

//load the fat from disk to memory
int fat_read_from_disk();

// allocate new blocks in FAt, starts from first free block.
// it logs an error if not enough free blocks to allocate and return -1.
uint32_t allocate_blocks(int blocks_to_allocate);

//functin to free blocks from fat
uint32_t release_blocks(int first_block);

//function to allocate more blocks if needed
void allocate_additional_blocks(uint32_t first_block, int blocks_to_allocate);

//function to get next block from fat
uint32_t get_next_block(int current_block);

// find first empty block in FAT
uint32_t find_free_block();

// check if a block is free, bby checking corresponding entry in the FAT
// 0 being free
int is_block_free(uint32_t block);

// return total number of free blocks,
// by counting the number of entries with zero values in FAT
uint32_t get_total_free_blocks();

#endif //__FAT_H__


