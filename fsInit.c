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

int initFileSystem(uint64_t number_of_blocks, uint64_t block_size) {
    printf("Initializing File System with %ld blocks with a block size of %ld\n", number_of_blocks, block_size);

    /* TODO: Add any code you need to initialize your file system. */

    int vcb_check = 0;

    // Initialize Volume Control Block. It returns -1 if there was an error during read, and 0 otherwise.
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
        int root_check = init_directory(block_size, NULL, "/");
        if (root_check == -1) {
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
        int root_check = load_directory(block_size, root_directory);

        if (root_check == -1) {
            printf("[ VCB INIT ] : Failed to load root directory.\n");
            free(vcb);
            return root_check;
        }

    }

    Current_Working_Directory* current_wd = malloc(sizeof (Current_Working_Directory));
    if (!current_wd) {
        printf("[ VCB INIT ] : Failed to allocate memory for current working directory.\n");
        free(vcb);
        return -1;
    }

    current_working_directory = current_wd;
    int ret_val = fs_setcwd("/");
    if (ret_val != 0) {
        printf("[ FS INIT ] : Failed to set current working directory.\n");
        free(vcb);
        free(current_wd);
        return ret_val;
    }



    /*
     int root_check = init_directory(block_size, NULL, "/");

     if (root_check == -1) {
         return root_check;
     }
     */

    return 0;
}

void exitFileSystem() {
    printf("System exiting\n");

    // Release memory of all structures loaded on system start
    free(vcb);
    vcb = NULL;

    free(fat_array);
    fat_array = NULL;

    free(root_directory);
    root_directory = NULL;

    free(current_working_directory);
    current_working_directory = NULL;
}
