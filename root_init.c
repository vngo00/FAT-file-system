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
#include <string.h>
#include <stdio.h>

#include "fsLow.h"
#include "mfs.h"
#include "vcb_.h"
#include "FAT.h"
#include "root_init.h"

// Initialize the current working directory and root directory
Current_Working_Directory* current_working_directory = NULL;
Directory_Entry* root_directory = NULL;

/*
 * Function to initialize a new directory.
 * Can also be used to create the root directory
*/
uint32_t init_directory(uint64_t block_size, Directory_Entry* parent_directory, char* dir_name) {

    vcb->entries_per_dir = block_size / sizeof(Directory_Entry);
    Directory_Entry* new_directory = malloc(block_size * vcb->entries_per_dir);
   
   // Check for memory allocation
    if (new_directory == NULL) {
        printf("Failed to allocate memory for new directory");
        return -1;
    }
    
    /*
	* If parent directory in NULL, use the root cluster
	* else find the first free block
	*/
    uint32_t first_cluster = parent_directory == NULL ? vcb->root_cluster : find_free_block() ; 

    // Mark all directory entries as free
    for (int i = 0; i < vcb->entries_per_dir; i++) {
        new_directory[i].dir_name[0] = 0xE5;
    }

    // Create the "." and ".." entries
    memset(new_directory[0].dir_name, ' ', NAME_MAX_LENGTH);
    memcpy(new_directory[0].dir_name, ".", strlen("."));

    // Set attributes for"." and ".." entries
    new_directory[0].dir_attr = new_directory[0].dir_attr |=1UL << 4;
    new_directory[0].dir_first_cluster = first_cluster;
    new_directory[0].dir_file_size = vcb->entries_per_dir * sizeof(Directory_Entry);

    memset(new_directory[1].dir_name, ' ', NAME_MAX_LENGTH);
    memcpy(new_directory[1].dir_name, "..", strlen(".."));
    new_directory[1].dir_attr = new_directory[0].dir_attr;
    new_directory[1].dir_first_cluster = parent_directory == NULL ? first_cluster : parent_directory->dir_first_cluster;
    new_directory[1].dir_file_size = new_directory[0].dir_file_size;

    // Try to write the new directory to disk
    uint32_t ret_value = LBAwrite(new_directory, 1, first_cluster);
    if (ret_value == -1) {
        free(new_directory);
        new_directory = NULL;
        printf("Something went wrong when writing the new directory");
        return ret_value;
    }

    // If we have a parent directory, we're creating a new directory entry in it.
    if (parent_directory != NULL) {
        Directory_Entry* new_entry = malloc(sizeof(Directory_Entry));
        memset(new_entry->dir_name, ' ', NAME_MAX_LENGTH);
        memcpy(new_entry->dir_name, dir_name, strlen(dir_name));
        new_entry->dir_attr = new_directory[0].dir_attr;
        new_entry->dir_first_cluster = first_cluster;
        new_entry->dir_file_size = new_directory[0].dir_file_size;
        // Might need to add the new directory entry to the parent directory.
        // add_directory_entry(parent_directory, new_entry);
    }

    return first_cluster;
}

/*
* This function loads a directory from the disk into memory.
* If the provided directory is NULL, it will load the root directory
*/
int load_directory(uint64_t block_size, Directory_Entry* directory) {
    // Handle root directory or non-root directory
    if (directory == NULL) {
      
        root_directory = malloc(block_size * vcb->entries_per_dir);
        // Check for memory allocation
        if (root_directory == NULL) {
            printf("Failed to allocate memory for root directory");
            return -1;
        }
        LBAread(root_directory, 1, vcb->root_cluster);
    } else {

        Directory_Entry* new_directory = malloc(block_size * vcb->entries_per_dir);
           // Check for memory allocation
        if (new_directory == NULL) {
            printf("Failed to allocate memory for directory");
            return -1;
        }
        LBAread(new_directory, 1, directory->dir_first_cluster);
    }

    return 0;
}

/*
* This function clears the current working directory
*/
void clear_current_working_directory() {
    memset(current_working_directory->path, '\0', sizeof(current_working_directory->path));
    current_working_directory->cluster = -1;
}
