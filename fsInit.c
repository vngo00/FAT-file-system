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

int initFileSystem(uint64_t number_of_blocks, uint64_t block_size) {
    printf("Initializing File System with %ld blocks with a block size of %ld\n", number_of_blocks, block_size);
   bytes_per_block = block_size;
    /* TODO: Add any code you need to initialize your file system. */

    int vcb_check = 0;

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
    } else {
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

void exitFileSystem() {
    printf("System exiting\n");

    free(vcb);
    vcb = NULL;

    free(fat_array);
    fat_array = NULL;

    free(root_directory);
    root_directory = NULL;

    free(cwd);
    cwd = NULL;
}
