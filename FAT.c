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
* File: FAT.c
*
* Description:  free space management
**************************************************************/


#include "FAT.h"
#include "vcb_.h"

int *fat_array; // cached FAT in mem
int blocks_per_fat;
int total_blocks;
int total_free_blocks;

int init_FAT(uint64_t num_blocks, uint64_t block_size) {
	fat_array =(int *) malloc(num_block * block_size); // 19531 * 4 = 78124 bytes
	//fat_array = (int *) mallloc(num_blocks * sizeof(int));
	total_blocks = num_blocks;
	total_free_blocks = total_blocks - RESERVED_BLOCKS;
	// set reserved blocks to occupied
	// -1 being occupied for now
	int i;
	for ( i =0; i < RESERVED_BLOCKS; i++) {
		fat_array[i] = -1;
	}

	// 0 being free
	for( ; i < num_blocks ; i++) {
		fat_array[i] = 0;
	}
	
	return 0 

}
/*
 * volume already initialized load FAT from disk
 */
int read_FAT_from_disk(uin64_t num_blocks, uint64_t block_size) {
	fat_array = (int *) malloc(num_blocks * block_size);
	total_blocks = num_blocks;
	
	// load FAT from disk
	LBAread( (void *) fat_array, vcb->FAT_size_32, vcb->FAT_start);

	// check the total number of free blocks
	total_free_blocks = 0 ;
	int i = RESERVED_BLOCKS;
	for( ; i < vcb->total_blocks_32; i++) {
		if (fat_array[i] == 0)
			total_free_blocks++;
	}

	return 0;
}

/*
 * allocate one free block to caller
 */
int allocate_blocks(int blocks) {
	if (blocks > total_free_blocks) return -1; // not enoug free blocks
	
	int head =-1;
	int curr = -1;
	for (int i = RESERVED_BLOCKS; i < total_blocks; i++) {
		if ( fat_array[i] == 0) {
			if ( head == -1) {
				head = i;
				curr = head;
			}
			else {
				fat_array[curr] = i;
				curr = i;
			}
			block--;
		}
	}
	fat_array[curr] = EOF;
	total_free_blocks -= blocks;
	LBAwrite( (void *) fat_array, vcb->FAT_size_32, vcb->FAT_start);
	return head;
}




/*
 * extend the size of a file by one block
 */
int allocate_additional_bocks(uint32_t start_block, uint32_t blocks) {
	int new_block = allocate_blocks(blocks);
	if (new_block == -1) {
		return -1 // no more free block available
	}
	prev = start_block;
	curr = fat_array[start_block];

	while (curr != EOF) {
		prev = curr;
		curr = fat_array[prev];
	}

	fat_array[prev] = new_block;
	total_free_blocks -= blocks;
	LBAwrite( (void *) fat_array, vcb->FAT_size_32, vcb->FAT_start);


	return 0; // 0 being successful;
}


/*
 * free all blocks of a removed file
 */
uint32_t release_blocks(int start_blocks) {
	int prev = start_blocks;
	int curr = fat_array[prev];

	while (curr != EOF) {
		fat_array[prev] = 0;
		prev = curr;
		curr = fat_array[prev];
		total_free_blocks++;
	}
	
	fat_array[prev] = o;
	total_free_blocks++;

	LBAwrite( (void *) fat_array, vcb->FAT_size_32, vcb->FAT_start);
	return 0 //not sure what to return here
}

uint32_t get_next_block(int current_block) {
	if (current_block >= total_blocks || current_block < RESERVED_BLOCKS) return -1;
	return fat_array[current_block];
}
