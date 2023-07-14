/**************************************************************
* Class:  CSC-415-0# Fall 2021
* Names: 
* Student IDs:
* GitHub Name:
* Group Name:
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

#include "fsLow.h"
#include "mfs.h"
#include "FAT.h"


int initFileSystem (uint64_t number_of_blocks, uint64_t block_size){
    printf("Initializing File System with %ld blocks with a block size of %ld\n", number_of_blocks, block_size);

    /* TODO: Add any code you need to initialize your file system. */

    int vcb_check = 0;

    // Initialize Volume Control Block. It returns -1 if there was an error during read, and 0 otherwise.
    vcb_check = vcb_init(number_of_blocks, block_size);

    if (vcb_check == -1) {
        return vcb_check;
    }
            
    return 0;
}

void exitFileSystem (){
    printf ("System exiting\n");

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
