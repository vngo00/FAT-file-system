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
	fo (int i = vcb->reserved_blocks_count + 1; i < vcb->total_blocks_32; i++) {
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

// read FAT from disk and stores it in memory
int fat_read_from_disk() {
	printf("read %d blocks of size %d\n", 154, 512);
	
	if (fat_array) {
		free(fat_array);
	}

	fat_array = (int *) malloc(154* 512);
	if (fat_array == NULL) {
		fprintf(stderr, "Failed to allocate memory for FAT. \n");
		return -1;
	}

	if (LBARread(fat_array, 154, FAT_BLOCK_START_LOCATION) == -1) {
		fprintf(stderr, "Failed to read FAT from disk but was able to allocate memeory.\n");
		free(fat_array);
		fat_array = NULL;
		return -1;
	}
	printf("read FAT from disk and was able to allocate memory.\n");

	return 0;
}

/*
 * allocate blocks to caller
 */
uint32_t allocate_blocks(int blocks_to_allocate) {
	
	uint32_t first_block =  find_first_empty_block_in_fat();
	int previous = -1;
	int blocks_allocated = 0;

	for (int curr = first_block; blocks_allocated < blocks_to_allocate && curr < vcb->total_blocks_32; curr++) {
		if (fat_array[curr] == 0) {
			if (previous != -1) {
				fat_array[previous] = curr;
			}
			previous  = curr;
			blocks_allocated++;
		}
	}

	if (blocks_allocated < blocks_to_allocate) {
		fprintf(stderr, "Not enough bblocks to allocate. \n");
		return -1;
	}

	fat_array[previous] = -1;
	update_fat_on_disk();

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

	if (first_new_block == -1) {
		fprintf(stderr, "Failed to allocate additional blocks.\n");
		return;
	}

	// link the end of current chain to beginning of new one
	fat_array[curr_index] = first_new_block;
	update_fat_on_disk();
}


/*
 * free given blocks in chain starting at firstBlock
 */
uint32_t release_blocks(int first_blocks) {

	int curr_index = first_block;
	int blocks_freed = 0;
	
	while (fat_array[curr_index] != -1) {
		int next_index = fat_array[curr_index];
		fat_array[curr_index] = 0;
		blocks_freed++;
		curr_index = next_index;
	}


	update_fat_on_disk();
	return blocks_freed;
}


// retrieves the next block in the chain for a specific block
uint32_t get_next_block(int current_block) {
	int next = fat_array[current_block];
	return next != -1 ? next : -1;
}




// find a free block by calling the function to find the first empty block in the FAT.
uint32_t find_free_block() {
	uint32_t free_block = find_first_empty_block_in_fat();
	if (free_block == -1) {
		fprintf(stderr, "No free blocks available.\n");
		return -1;
	}

	return free_block;
}

// check if a block is free by checking the corresponding entry in the FAT.
int is_block_free(uint32_t block) {
	return fat_array[block] == 0;
}

// return the total number of free block by counting the number of zero entries in the FAT
uint32_t get_total_free_blocks() {
	uint32_t free_blocks = 0;
	for (int i = 0; i < vcb->total_blocks_32; i++) {
		if (fat_array[i] == 0) {
			free_blocks++;
		}
	}
	return free_blocks;
}

