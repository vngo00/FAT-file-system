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


int init_FAT(uint64_t num_blocks, uint64_t block_size) {
	//fat_array = malloc(num_block * block_size); // 19531 * 4 = 78124 bytes
	fat_array = (int *) mallloc(num_blocks * sizeof(int));

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
