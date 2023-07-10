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

int *fat_array; // cached FAT in mem
int blocks_per_fat;
int total_blocks;


int init_FAT(uint64_t num_blocks, uint64_t block_size) {
	//fat_array = malloc(num_block * block_size); // 19531 * 4 = 78124 bytes
	fat_array = (int *) mallloc(num_blocks * sizeof(int));
	total_blocks = num_blocks;
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
 * allocate one free block to caller
 */
int allocate_blocks(void) {
	int i;
	for (i = RESERVED_BLOCKS; i < total_blocks; i++) {
		if (fat_array[i] == 0) return i;
	}
	return -1 // no free block avaiable
}


/*
 * extend the size of a file by one block
 */
int allocate_additional_bock(uint32_t start_block) {
	int new_block = allocate_blocks();
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
	fat_array[new_block] = EOF;


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
	}
	
	fat_array[prev] = o;


	return 0 //not sure what to return here
}
