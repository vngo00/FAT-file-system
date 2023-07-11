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
#include "mfs.h"

Current_Working_Directory* current_working_directory = NULL;
Directory_Entry* root_directory = NULL;



uint32_t init_directory(uint64_t block_size, Directory_Entry* parent_directory, char* dir_name) {
    vcb->entries_per_dir = block_size / sizeof(Directory_Entry);
    Directory_Entry* new_directory = malloc(block_size * vcb->entries_per_dir);
    if (new_directory == NULL) {
        printf("Failed to allocate memory for new directory");
        return -1;
    }
    uint32_t first_cluster = parent_directory == NULL ? vcb->root_cluster : find_free_block() ; 

    //mark all entries as free at first
    for (int i = 0; i < vcb->entries_per_dir; i++) {
        new_directory[i].dir_name[0] = 0xE5;
    }

    //dot and dot dot entries
    memset(new_directory[0].dir_name, ' ', NAME_MAX_LENGTH);
    memcpy(new_directory[0].dir_name, ".", strlen("."));

    

    new_directory[0].dir_attr = new_directory[0].dir_attr |=1UL << 4;
    new_directory[0].dir_first_cluster = first_cluster;
    new_directory[0].dir_file_size = vcb->entries_per_dir * sizeof(Directory_Entry);

    memset(new_directory[1].dir_name, ' ', NAME_MAX_LENGTH);
    memcpy(new_directory[1].dir_name, "..", strlen(".."));

    
    new_directory[1].dir_attr = new_directory[0].dir_attr;
    new_directory[1].dir_first_cluster = parent_directory == NULL ? first_cluster : parent_directory->dir_first_cluster;
    new_directory[1].dir_file_size = new_directory[0].dir_file_size;

    uint32_t ret_value = LBAwrite(new_directory, 1, first_cluster);

    if (ret_value == -1) {
        free(new_directory);
        new_directory = NULL;
        printf("Something went wrong when writing the new directory");
        return ret_value;
    }

    if (parent_directory != NULL) {
        Directory_Entry* new_entry = malloc(sizeof(Directory_Entry));


    memset(new_entry->dir_name, ' ', NAME_MAX_LENGTH);
    memcpy(new_entry->dir_name, dir_name, strlen(dir_name));

        
        new_entry->dir_attr = new_directory[0].dir_attr;
        new_entry->dir_first_cluster = first_cluster;
        new_entry->dir_file_size = new_directory[0].dir_file_size;
    }

    return first_cluster;
}


int load_root_directory(uint64_t block_size) {
    root_directory = malloc(block_size *vcb->entries_per_dir);
        if (root_directory == NULL) {
        printf("Failed to allocate memory for root directory");
        return -1;
    }
    LBAread(root_directory, 1, vcb->root_cluster);

    return 0;
}

void clear_current_working_directory() {
    memset(current_working_directory->path, '\0', sizeof(current_working_directory->path));
    current_working_directory->cluster = -1;
}