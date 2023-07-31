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
 * File: fsInit.c
 *
 * Description: Main driver for file system assignment.
 *
 * This file is where you will start and initialize your system
 *
 **************************************************************/
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>


#include "mfs.h"
#include "FAT.h"

int bytes_per_block;

extern Directory_Entry *root_directory;

/**
 * This Function is used to initalize the file system
 *
 * @param number_of_blocks - The number of blocks in the file system
 * @param block_size - THe block size of each block in the file system
 *
 * @return - the next block in the chain from the fat array
 *         
 */
int initFileSystem(uint64_t number_of_blocks, uint64_t block_size) {
    printf("Initializing File System with %ld blocks with a block size of %ld\n", number_of_blocks, block_size);
   bytes_per_block = block_size;
    /* TODO: Add any code you need to initialize your file system. */

    int vcb_check = 0;

    //VCB not initalized at start, so you try to initalize
    if (!vcb_is_init()) {
        printf("[ FS INIT ] : VCB not initialized. Attempting initialization...\n");

        vcb_check = vcb_init(number_of_blocks, block_size);
        if (vcb_check == -1) {
            return vcb_check;
        }

        int fat_check = fat_init(vcb->total_blocks_32, block_size);

        if (fat_check == -1) {
            return fat_check;
        }
        root_directory = init_directory(block_size, NULL, "");
	current_directory = root_directory;
        if (root_directory == NULL) {
            printf("[ FS INIT ] : Failed to initialize root directory.\n");

        }
    } else { //otherwise vcb already intialized so start without initalizing it
        printf("[ FS INIT ] : VCB already initialized. Loading from disk...\n");

        vcb_check = vcb_read_from_disk(vcb);
        if (vcb_check == -1) {
            printf("[ FS INIT ] : Failed to read VCB from disk.\n");
            free(vcb);
            return vcb_check;
        }
        fat_read_from_disk();
        int root_check = load_root();

        if (root_check == -1) {
            printf("[ VCB INIT ] : Failed to load root directory.\n");
            free(vcb);
            return root_check;
        }

    }
	

    return 0;
}

/**
 * This Function is used to exit the file system
 *
 *
 * @return - void
 *         
 */
void exitFileSystem() {
    printf("System exiting\n");

    free(vcb);
    vcb = NULL;

    free(fat_array);
    fat_array = NULL;

	
    if (current_directory != root_directory) {
	    free(current_directory);
	    current_directory = NULL;
    }

    free(root_directory);
    root_directory = NULL;

    free(cwd);
    cwd = NULL;
}
