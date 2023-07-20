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



// updates the FAT on disk. It uses the LBAwrite method to perform the write operation
// if write fails, logs an error message.


// initilizes the FAT. It calculates the size of the FAT in bytes and blocks.
// After that, it allocates memory for the FAT and sets up the initial FAT structure.
// If memory allocation fails, it logs an error message and return -1.

int fat_init(uint64_t number_of_blocks, uint64_t block_size) {
    printf("[ FAT INIT ] : Initializing FAT with %ld blocks and block size of %ld\n", number_of_blocks, block_size);

    int bytes_per_fat = number_of_blocks * sizeof (uint32_t);
    printf("[ FAT INIT ] : Calculated bytes_per_fat as %d\n", bytes_per_fat);

    int blocks_per_fat = to_blocks(bytes_per_fat);
    printf("[ FAT INIT ] : Calculated blocks_per_fat as %d\n", blocks_per_fat);

    printf("[ FAT INIT ] : Trying to allocate %d blocks of size %d\n", blocks_per_fat, block_size);

    fat_array = (int*) malloc(blocks_per_fat * block_size);

    if (!fat_array) {
        fprintf(stderr, "[ FAT INIT ] : Failed to allocate memory for FAT.\n");
        return -1;
    }

    printf("[ FAT_INIT ] : Starting to populate fat_array.\n");
    for (int i = 0; i < number_of_blocks; i++) {
        if (i == 0 || i == number_of_blocks - 1) {
            fat_array[i] = EOF_BLOCK;
            //            printf("[ FAT_INIT ] : Setting EOF_BLOCK for block %d.\n", i);
            continue;
        }
        if (i <= 154) {
            fat_array[i] = RESERVED_BLOCK;
            //                printf("[ FAT_INIT ] : Setting RESERVED_BLOCK for block %d.\n", i);
        } else {
            fat_array[i] = FREE_BLOCK;
            //              printf("[ FAT_INIT ] : Setting FREE_BLOCK for block %d.\n", i);
        }

    }

    printf("[ FAT INIT ] : FAT is being updated on disk\n");
    update_fat_on_disk();

    printf("[ FAT INIT ] : FAT initialization successful\n");
    return 0;
}

// read FAT from disk and stores it in memory

int fat_read_from_disk() {
    printf("[ FAT READ ] : Trying to read %d blocks of size %d\n", vcb->FAT_size_32, vcb->bytes_per_block);

    fat_array = (int*) malloc(vcb->FAT_size_32 * vcb->bytes_per_block);
    if (fat_array == NULL) {
        fprintf(stderr, "[ FAT READ ] : Failed to allocate memory for FAT.\n");
        return -1;
    }

    if (LBAread(fat_array, vcb->FAT_size_32, FAT_BLOCK_START_LOCATION) != vcb->FAT_size_32) {
        fprintf(stderr, "[ FAT READ ] : Failed to read FAT from disk but was able to allocate memory.\n");
        free(fat_array);
        fat_array = NULL;
        return -1;
    }

    printf("[ FAT READ ] : Successfully read FAT from disk and allocated memory.\n");
    return 0;
}

/*
 * This function searches the fat array for a free block and 
 * returns its index otherwise returns -1 indicating none were found.
 */
uint32_t find_free_block() {
    printf("[ FIND FREE BLOCK ] : Searching for a free block...\n");
    int block_index = -1;

    for (int i = vcb->reserved_blocks_count; i < vcb->total_blocks_32; i++) {
        if (fat_array[i] == FREE_BLOCK) {
            printf("[ FIND FREE BLOCK ] : Free block found at index %d\n", i);
            block_index = i;
            return block_index;
        }
    }
    fprintf(stderr, "[ FIND FREE BLOCK ] : No free blocks available.\n");
    return block_index;
}

/*
 * Allocate a given number of blocks, returns the starting block.
 */
uint32_t allocate_blocks(int blocks_needed) {
    printf("[ ALLOCATE_BLOCKS ] : Allocating blocks_needed = %d.\n", blocks_needed);
    int *blocks_found = malloc(blocks_needed * sizeof (uint32_t));
    int blocks = 0;

    while (blocks < blocks_needed) {
        printf("[ ALLOCATE_BLOCKS ] : Searching for free block.\n");
        int free_block_index = find_free_block();
        if (free_block_index > -1) {
            printf("[ ALLOCATE_BLOCKS ] : Found free block at index %d.\n", free_block_index);
            blocks_found[blocks] = free_block_index;
            blocks++;
        }
    }
    if (blocks_needed != 1) {
        for (int i = 0; i < blocks; i++) {
            printf("[ ALLOCATE_BLOCKS ] : Linking block at index %d to block at index %d.\n", blocks_found[i], blocks_found[i + 1]);
            fat_array[blocks_found[i]] = fat_array[blocks_found[i + 1]]; //linking the blocks
            if (i == blocks - 1) {
                printf("[ ALLOCATE_BLOCKS ] : Assigning EOF_BLOCK at index %d.\n", blocks_found[0]);
                fat_array[blocks_found[i]] = EOF_BLOCK;

            }
        }
    }

    printf("[ ALLOCATE_BLOCKS ] : Updating FAT.\n");
    update_fat_on_disk();

    return blocks_found[0];
}

/*
 * extend the size of a file by one block
 */
void allocate_additional_bocks(uint32_t first_block, int blocks_to_allocate) {
    int curr_index = first_block;

    //Get to the EOF BLOCK
    while (fat_array[curr_index] != EOF_BLOCK) {
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
 * Starting from the given first block this function
 * frees blocks using the index stored at the given position
 * in the fat until eof is reached. Returns the number of blocks freed
 */
uint32_t release_blocks(int first_block) {

    int curr_index = first_block;
    int blocks_freed = 0;

    while (fat_array[curr_index] != EOF_BLOCK) {
        int next_index = fat_array[curr_index];
        fat_array[curr_index] = FREE_BLOCK;
        blocks_freed++;
        curr_index = next_index;
    }


    update_fat_on_disk();
    return blocks_freed;
}

/*
 * Function retrieves the next block in the chain
 */
uint32_t get_next_block(int current_block) {
    int next = fat_array[current_block];
    return next != EOF_BLOCK ? next : EOF_BLOCK;
}

/*
 * Counts free blocks in the fat
 */
uint32_t get_total_free_blocks() {
    uint32_t free_blocks = 0;
    for (int i = vcb->reserved_blocks_count; i < vcb->total_blocks_32; i++) {
        if (fat_array[i] == FREE_BLOCK) {
            free_blocks++;
        }
    }
    return free_blocks;
}

/*
 * Function that computes blocks from 
 * bytes given
 */
int to_blocks(int bytes) {
    printf("[ TO_BLOCKS ] : Converting bytes = %d to blocks.\n", bytes);
    return (bytes + (vcb->bytes_per_block - 1)) / vcb->bytes_per_block;
}

void update_fat_on_disk() {
    if (LBAwrite(fat_array, vcb->FAT_size_32, FAT_BLOCK_START_LOCATION) != vcb->FAT_size_32) {
        fprintf(stderr, "Failed to update FAT on disk.\n");
    }
}
