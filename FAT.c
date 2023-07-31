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

/**
 * This Function is used to initalize the FAT system
 *
 * @param number_of_blocks - A uint64_t of number of blocks needed for the fat
 * @param block_size - A uint64_t of block size for each indiviual block for the fat
 *
 * @return - if sucessfully initlized the fat return 0
 *         - if failed to allocate memory for fat return -1
 *         
 */

int fat_init(uint64_t number_of_blocks, uint64_t block_size) {
    printf("[ FAT INIT ] : Initializing FAT with %ld blocks and block size of %ld\n", number_of_blocks, block_size);

    //Calulating the bytes per fat using the numbers of blocks and 4 bytes
    int bytes_per_fat = number_of_blocks * sizeof (uint32_t);
    printf("[ FAT INIT ] : Calculated bytes_per_fat as %d\n", bytes_per_fat);

    //blocks per fat, basically converting the bytes per fat to block representation
    int blocks_per_fat = to_blocks(bytes_per_fat);
    printf("[ FAT INIT ] : Calculated blocks_per_fat as %d\n", blocks_per_fat);

    printf("[ FAT INIT ] : Trying to allocate %d blocks of size %d\n", blocks_per_fat, block_size);

    //allocating memory using the two sizes for the fat array
    fat_array = (int*) malloc(blocks_per_fat * block_size);

    if (!fat_array) {
        fprintf(stderr, "[ FAT INIT ] : Failed to allocate memory for FAT.\n");
        return -1;
    }

    printf("[ FAT_INIT ] : Starting to populate fat_array.\n");
    //Populate the fat array
    for (int i = 0; i < number_of_blocks; i++) {
        if (i == 0 || i == number_of_blocks - 1) {
            //Setting the EOF_Block for the block
            fat_array[i] = EOF_BLOCK;
            //            printf("[ FAT_INIT ] : Setting EOF_BLOCK for block %d.\n", i);
            continue;
        }
        if (i <= 153) {
            //Setting if the block is reserved
            fat_array[i] = RESERVED_BLOCK;
            //                printf("[ FAT_INIT ] : Setting RESERVED_BLOCK for block %d.\n", i);
        } else {
            //Otherwise set it to a free block
            fat_array[i] = FREE_BLOCK;
            //              printf("[ FAT_INIT ] : Setting FREE_BLOCK for block %d.\n", i);
        }

    }

    printf("[ FAT INIT ] : FAT is being updated on disk\n");
    update_fat_on_disk();

    printf("[ FAT INIT ] : FAT initialization successful\n");
    return 0;
}

/**
 * This Function is used to read FAT from disk and stores it in memory
 *
 * @param path - A directory entry pointer dir that we want to free
 *
 * @return - succesfully read fat from disk and allocated memory return 0
 *         - if failed to allocate memory for fat return -1
 *         
 */
int fat_read_from_disk() {
    printf("[ FAT READ ] : Trying to read %d blocks of size %d\n", vcb->FAT_size_32, vcb->bytes_per_block);

    //allocating memory for the fat to be able to read to disk
    fat_array = (int*) malloc(vcb->FAT_size_32 * vcb->bytes_per_block);
    if (fat_array == NULL) {
        fprintf(stderr, "[ FAT READ ] : Failed to allocate memory for FAT.\n");
        return -1;
    }

    //reading from diskk
    if (LBAread(fat_array, vcb->FAT_size_32, FAT_BLOCK_START_LOCATION) != vcb->FAT_size_32) {
        fprintf(stderr, "[ FAT READ ] : Failed to read FAT from disk but was able to allocate memory.\n");
        free(fat_array);
        fat_array = NULL;
        return -1;
    }

    printf("[ FAT READ ] : Successfully read FAT from disk and allocated memory.\n");
    return 0;
}

/**
 * This Function is used to searches the fat array for a free block
 *
 *
 * @return - succesful return index
 *         - if otherwise return -1
 *         
 */
uint32_t find_free_block() {
//    printf("[ FIND FREE BLOCK ] : Searching for a free block...\n");
    int block_index = -1;

    for (int i = vcb->reserved_blocks_count; i < vcb->total_blocks_32; i++) {
        if (fat_array[i] == FREE_BLOCK) {
   //        printf("[ FIND FREE BLOCK ] : Free block found at index %d\n", i);
            block_index = i;
            fat_array[i] = RESERVED_BLOCK;
            return block_index;
        }
    }
    fprintf(stderr, "[ FIND FREE BLOCK ] : No free blocks available.\n");
    return block_index;
}

/**
 * This Function is used to Allocate a given number of blocks, returns the starting block.
 *
 * @param blocks_needed - A int the contains how many blocks needed for allocating
 *
 * @return - succesfully allocated return the starting block
 *         - if failed to allocate blocks return -1
 *         
 */
uint32_t allocate_blocks(int blocks_needed) {
    printf("[ ALLOCATE_BLOCKS ] : Allocating blocks_needed = %d.\n", blocks_needed);

    if (blocks_needed <= 0 || blocks_needed > get_total_free_blocks()) {
        printf("[ ALLOCATE_BLOCKS ] : Invalid blocks_needed.\n");
        return -1;
    }

    uint32_t *blocks_found = (uint32_t *)malloc(blocks_needed * sizeof(uint32_t));
    if (blocks_found == NULL) {
        printf("[ ALLOCATE_BLOCKS ] : Failed to allocate memory.\n");
        return -1;
    }

    int blocks = 0;
    while (blocks < blocks_needed) {
        int free_block_index = find_free_block();
        if (free_block_index == -1) {
            printf("[ ALLOCATE_BLOCKS ] : No more free blocks.\n");
            free(blocks_found);
            return -1;
        }
        blocks_found[blocks] = free_block_index;
        blocks++;
    }

    uint32_t start_block = blocks_found[0];

    if (blocks_needed != 1) {
        for (int i = 0; i < blocks - 1; i++) { 
            if (blocks_found[i] == vcb->total_blocks_32 - 1) {
                printf("[ ALLOCATE_BLOCKS ] : Last block in FAT, can't link to the next block.\n");
                break;
            }
            fat_array[blocks_found[i]] = blocks_found[i + 1];
        }
    }

    fat_array[blocks_found[blocks - 1]] = EOF_BLOCK;

    printf("[ ALLOCATE_BLOCKS ] : Updating FAT.\n");
    update_fat_on_disk();

    free(blocks_found);

    return start_block;
}

/**
 * This Function is used to Allocate a given number of blocks, returns the void
 *
 * @param first_block - A first block in chain
 * @param blocks_to_allocate - how many blocks you need to allocate
 *
 * @return - void
 *         
 */
void allocate_additional_blocks(uint32_t first_block, int blocks_to_allocate) {
    int curr_index = first_block;

    //Get to the EOF BLOCK
    while (fat_array[curr_index] != EOF_BLOCK) {
        curr_index = fat_array[curr_index];
    }

    // allocate new chain with desired amount of blocks
    uint32_t first_new_block = allocate_blocks(blocks_to_allocate);

    //if fail to allocate additional blocks
    if (first_new_block == -1) {
        fprintf(stderr, "Failed to allocate additional blocks.\n");
        return;
    }

    // link the end of current chain to beginning of new one
    fat_array[curr_index] = first_new_block;
    update_fat_on_disk();
}

/**
 * This Function is used to frees blocks using the index stored at the given position in fat unti EOF
 *
 * @param first_block - A first block in chain
 *
 * @return - amount of blocks that you freed
 *         
 */
uint32_t release_blocks(int first_block) {

    int curr_index = first_block;
    int blocks_freed = 0;

    //looping to get each block and free each one
    while (fat_array[curr_index] != EOF_BLOCK) {
        int next_index = fat_array[curr_index];
        fat_array[curr_index] = FREE_BLOCK;
        blocks_freed++;
        curr_index = next_index;
    }

    //updating the fat after freeing the blocks
    update_fat_on_disk();
    return blocks_freed;
}

/**
 * This Function is used to retrieve the next block in the chain
 *
 * @param current_block - THe current block in chain
 *
 * @return - the next block in the chain from the fat array
 *         
 */
uint32_t get_next_block(int current_block) {
    return fat_array[current_block];
}

/**
 * This Function is used to retrieve the Count of free blocks in the fat
 *
 *
 * @return - the total amount of free blocks in fat
 *         
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

/**
 * This Function is used to compute blocks from blocks given
 *
 * @param current_block - The amount of bytes you want to convert
 *
 * @return - the converted amount of blocks
 *         
 */
int to_blocks(int bytes) {
    printf("[ TO_BLOCKS ] : Converting bytes = %d to blocks.\n", bytes);
    return (bytes + (vcb->bytes_per_block - 1)) / vcb->bytes_per_block;
}

/**
 * This Function is used to update fat
 *
 * @return - void
 *         
 */
void update_fat_on_disk() {
    //if not equal to vcb->FAT_size_32, then it means that the update on fat array has gone wrong
    if (LBAwrite(fat_array, vcb->FAT_size_32, FAT_BLOCK_START_LOCATION) != vcb->FAT_size_32) {
        fprintf(stderr, "Failed to update FAT on disk.\n");
    }
}
