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

// necessary libraries and modules for the file
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>



// Including headers relevant to this file's operaitons.
#include "fsLow.h"
#include "mfs.h"
#include "FAT.h"
#include "vcb_.h"

// Declaration of the file allocation table array and the blocks per FAT variable.
int * fat_array = NULL;
int blocks_per_fat = -1;

// this function is used to identify the first free block in the FAT.
// Iterate through each block until it finds a block not in use.
int find_first_empty_block_in_fat() {
	fo (int i = vcv->reserved_blocks_count + 1; i < vcb->total_blocks_32; i++) {
		if (fat_array[i] == 0) {
			return i;
		}
	}
	return -1;
}

// updates the FAT on disk. It uses the LBAwrite method to perform the write operation
// if write fails, logs an error message.
void update_fat_on_disk() {
	if (LBAwrite(fat_array, block_per_fat, FAT_BLOCK_START_LOCATION) == -1) {
		fprintf(stderr, "Failed to update FAT on disk.\n");
	}
}


// initilizes the FAT. It calculates the size of the FAT in bytes and blocks.
// After that, it allocates memory for the FAT and sets up the initial FAT structure.
// If memory allocation fails, it logs an error message and return -1.
int fat_init(uint64_t number_of_blocks, uint64_t block_size) {
	printf("Initializing FAT with %ld blocks and block size of %ld\n", number_of_blocks, block_size);

	int bytes_per_fat = number_of_blocks * sizeof(int);
	printf("Calculated bytes_per_fat as %d\n", bytes_per_fat);

	blocks_per_fat = (bytes_per_fat + block_size - 1 ) / block_size; // round up
	printf("Calculated blocks_per_fat as %dn", blocks_per_fat);

	printf("Trying to allocate %d blocks of size %d\n", blocks_per_fat, block_size);


        fat_array = (int *)malloc(blocks_per_fat * block_size);


    	if (!fat_array) {
		fprintf(stderr, "Failed to allocate memory for FAT.\n");
		return -1;
	}
	
	for(int i = 0; i < number_of_blocks; i++) {
		fat_array[i] = i == number_of_blocks -1 ? -1 : i +1;
	}
	
	update_fat_on_disk();
	return 0; 

}

/*
 * volume already initialized load FAT from disk
 */
int fat_read_from_disk(uin64_t num_blocks, uint64_t block_size) {
    fat_array = malloc(vcb->FAT_size_32 * vcb->bytes_per_block);
    return LBAread(fat_array, vcb->FAT_size_32, FAT_BLOCK_START_LOCATION);
}

/*
 * allocate blocks to caller
 */
uint32_t allocate_blocks(int blocks_to_allocate) {
	
LBAwrite( (void *) fat_array, vcb->FAT_size_32, vcb->FAT_start);
	uint32_t first_block = -1; // first block of alloaction chain
	int previous = -1;
	int blocks_allocated = 0;
	int curr = 0; // index current block in the chain;

	while (1) {
		if (fat_array[curr] == 0)( { // 0 means lock is free
				if (block_allocated == 0) { // found first free block
					first_block = curr;

					if (blocks_to_allocate == 1) {
						// if we found first free block
						// andd we only need one, we are done
						break;
					}
				}

				if (previous == -1) {
					// nothing points to first block of chain
					previous = curr;
				} else {
					// link the chain
					fat_array[previous] = curr;
					previous = curr;
				}

				blocks_allocated++; // track how many blocks we've allocated
				if (blocks_allocated == blocks_to_allocate) break; // done after allocating all blocks we need
		}
		curr++; // move on to check next block
	}

	fat_array[curr] = -1; // end of chain signal
	LBAwrite(fat_array, vcb->FAT_size_32, FAT_BBLOCK_START_LOCATION);
	return first_block;
}




/*
 * extend the size of a file by one block
 */
void allocate_additional_bocks(uint32_t first_block, int blocks_to_allocate) {
	// need to find the end of the chain first
	int curr_index = first_block;

	// while we're not at the end of the chain
	while (fat_array[curr_index] != -1) {
		// step into the next block
		curr_index = fat_array[curr_index];
	}

	// allocate new chain with desired amount of blocks
	uint32_t first_new_block = allocate_blocks(blocks_to_allocate);

	// link the end of current chain to beginning of new one
	fat_array[curr_index] = first_new_block;
}


/*
 * free given blocks in chain starting at firstBlock
 */
uint32_t release_blocks(int first_blocks) {

	int curr_index = first_bblock;
	int next_index = 0;
	int bblocks_freed = 0;

	//k eep releasing until the end of chain
	while (1) {
		if (fat_array[curr_index] == -1) { // at end of chain
			fat_array[curr_index] = 0;
			blocks_freed++;
			break;
		}

		// find next block to free
		next_index = fat_array[curr_index];
		fat_array[curr_index] = 0; //
		blocks_freed++;
		curr_index = next_index; // move up the chain
	}

	LBAwrite(fat_array, vcb->FAT_size_32, FAT_BBLOCK_START_LOCATION);
	return blocks_freed;
}

uint32_t get_next_block(int current_block) {
	int next = fat_array[current_block];
	if (next != -1){
		return next;
	}
	return -1;
}


uint32_t find_free_block() {
	int curr = 0; // start at the beginning of the FAT
	

	// loop through the entire FAT
	while (curr < vcb->total_blocks_32) {
		// if this block is free
		if (fat_array[curr] == 0) {
			return curr; // return the first free block
		}
		curr++; // move on to check next block
	}
	return -1; // return -1 if no free block is found
}

