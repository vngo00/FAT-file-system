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
* File: root_init.c
*
* Description: Implementation for init. of root directory.
**************************************************************/
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "root_init.h"
#include "fsLow.h"
#include "vcb_.h"

Current_Working_Directory *current_working_directory = NULL;
Directory_Entry* root_directory = NULL;

uint32_t init_root_directory(uint64_t block_size) {

}

int load_root_directory(uint64_t block_size){

}

void clear_current_working_directory() {
    
}