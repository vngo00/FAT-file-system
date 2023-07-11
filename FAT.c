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

int * fat_array = NULL;
int blocks_per_fat = -1;

int fat_init(uint64_t num_blocks, uint64_t block_size) {
	int bytes_per_fat = number_of_blocks * sizeof(int);
	blocks_per_fat = (bytes_per_fat + block_size - 1 ) / block_size; // round up
        fat_array = malloc(blocks_per_fat * block_size);


	fat_array[0] = -1; // vcb occupies block 0
    


	for (int i = 1; i <= blocks_per_fat; i++) {
		if(i == blocks_per_fat){
			fat_array[i] = -1; // -1 denotes end of the chain 
	        } else {
		        fat_array[i] = i + 1; // contiguous allocation for the FAT
		}
	}
 	 LBAwrite(fat_array, blocks_per_fat, FAT_BLOCK_START_LOCATION);
	return 1; 

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

