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
Current_Working_Directory *current_working_directory = NULL;
Directory_Entry *root_directory = NULL;

/*
 * Function to initialize a new directory.
 * Can also be used to create the root directory
 */
uint32_t init_directory(uint64_t block_size, Directory_Entry* parent_directory, char* dir_name) {
    printf("[ INIT DIRECTORY ]: Starting initialization of directory %s\n", dir_name);

    // Allocate memory for the new directory
    Directory_Entry* new_directory = malloc(sizeof(Directory_Entry));
    if (new_directory == NULL) {
        printf("[ INIT DIRECTORY ]: Failed to allocate memory for new directory");
        return -1;
    }
    printf("[ INIT DIRECTORY ]: Memory for new directory %s allocated successfully\n", dir_name);

    // Copy the directory name and set directory attribute
    strncpy(new_directory->dir_name, dir_name, strlen(dir_name));
    new_directory->dir_attr = new_directory->dir_attr |= 1UL << 4;
    printf("[ INIT DIRECTORY ]: Directory name set to %s and attribute set to %lu\n", new_directory->dir_name, new_directory->dir_attr);

    // Determine the first cluster of the new directory
    uint32_t first_cluster = parent_directory == NULL ? vcb->root_cluster : find_free_block();
    printf("[ INIT DIRECTORY ]: First cluster: %u\n", first_cluster);

    // Set the size and location of the entries array for the directory
    new_directory->dir_file_size = vcb->entries_per_dir * sizeof(Directory_Entry);
    new_directory->entries_array_location = find_free_block();
    new_directory->dir_first_cluster = first_cluster;
    printf("[ INIT DIRECTORY ]: Directory file size set to %lu and entries array location set to %lu\n", new_directory->dir_file_size, new_directory->entries_array_location);

    // Allocate memory for directory entries
    Directory_Entry* entries = malloc(vcb->entries_per_dir * sizeof(Directory_Entry));
    if (entries == NULL) {
        printf("[ INIT DIRECTORY ]: Failed to allocate memory for directory entries");
        free(new_directory);
        return -1;
    }
    printf("[ INIT DIRECTORY ]: Memory for directory entries allocated successfully\n");

    // Initialize entries with empty names
    for (int i = 2; i < vcb->entries_per_dir; i++) {
        entries[i].dir_name[0] = '\0';
    }

    // Set '.' and '..' entries
    Directory_Entry dot_entry, dot_dot_entry;
    dot_entry.dir_name[0] = '.';
    dot_entry.dir_name[1] = '\0';
    dot_dot_entry.dir_name[0] = '.';
    dot_dot_entry.dir_name[1] = '.';
    dot_dot_entry.dir_name[2] = '\0';

    dot_entry.dir_attr = dot_dot_entry.dir_attr = new_directory->dir_attr;
    dot_entry.dir_first_cluster = dot_dot_entry.dir_first_cluster = first_cluster;
    dot_entry.dir_file_size = dot_dot_entry.dir_file_size = new_directory->dir_file_size;
    dot_entry.entries_array_location = dot_dot_entry.entries_array_location = new_directory->entries_array_location;
    printf("[ INIT DIRECTORY ]: '.' and '..' entries set\n");

    entries[0] = dot_entry;
    entries[1] = dot_dot_entry;

    // If the new directory has a parent, link it to the parent
    if (parent_directory != NULL) {
        dot_dot_entry.dir_first_cluster = parent_directory->dir_first_cluster;
        Directory_Entry* parent_array = malloc(vcb->entries_per_dir * sizeof(Directory_Entry));
        LBAread(parent_array, (sizeof(parent_array) + vcb->bytes_per_block - 1) / vcb->bytes_per_block, parent_directory->entries_array_location);

        printf("[ INIT DIRECTORY ]: Reading parent directory entries\n");

        int i = 0;
        for (; i < DIRECTORY_MAX_LENGTH; i++) {
            if (strlen(parent_array[i].dir_name) == 0) {
                parent_array[i] = *new_directory;
                break;
            }
        }
        if (i == DIRECTORY_MAX_LENGTH) {
            printf("[ INIT DIRECTORY ]: Parent directory has no space for new directory.\n");
            free(parent_array);
            free(new_directory);
            free(entries);
            return -1;
        }

        printf("[ INIT DIRECTORY ]: Writing new directory to parent\n");
        LBAwrite(parent_array, (sizeof(Directory_Entry) * (DIRECTORY_MAX_LENGTH) + vcb->bytes_per_block - 1) / vcb->bytes_per_block, parent_directory->entries_array_location);

        free(parent_array);
    }

    printf("[ INIT DIRECTORY ]: Writing new directory to disk\n");
    uint32_t ret_value = LBAwrite(new_directory, (sizeof(Directory_Entry) + vcb->bytes_per_block - 1) / vcb->bytes_per_block, new_directory->dir_first_cluster);
    printf("[ INIT DIRECTORY ]: New Dir First Cluster %ld\n", new_directory->dir_first_cluster);
    if (ret_value == -1) {
        printf("[ INIT DIRECTORY ]: Something went wrong when writing the new directory");
        free(new_directory);
        new_directory = NULL;
        free(entries);
        return ret_value;
    }

    printf("[ INIT DIRECTORY ]: Writing directory entries to disk\n");
    printf("[ INIT DIRECTORY ]: Entries array location: %lu\n", new_directory->entries_array_location); // Print entries array location
    LBAwrite(entries, vcb->entries_per_dir * sizeof(Directory_Entry), new_directory->entries_array_location);
    printf("[ INIT DIRECTORY ]: Entries array written to disk\n"); // Print when entries array is written
    free(entries);
    free(new_directory);

    printf("[ INIT DIRECTORY ]: Directory %s successfully initialized\n", dir_name);

    return new_directory->dir_first_cluster;
}

/*
 * This function loads a directory from the disk into memory.
 * If the provided directory is NULL, it will load the root directory
 */
int load_directory(uint64_t block_size, Directory_Entry* directory) {
    printf("[ LOAD DIRECTORY ] : Loading directory...\n");
 int block_num = -1;
    // If the directory is NULL,  dealing with the root directory.
    if (directory == NULL) {
        printf("[ LOAD DIRECTORY ] : Directory is NULL. Allocating memory for root directory...\n");

        //  allocating memory for the root directory here.
        root_directory = malloc(block_size * vcb->entries_per_dir);

        // If couldn't get the memory,  just print an error message and return -1.
        if (root_directory == NULL) {
            printf("[ LOAD DIRECTORY ] : Failed to allocate memory for root directory.\n");
            return -1;
        }

        // reading the root directory from the disk here.
        printf("[ LOAD DIRECTORY ] : Reading root directory from disk...\n");
        LBAread(root_directory, 1, vcb->root_cluster);
        return 0;
    } else {
        printf("[ LOAD DIRECTORY ] : Directory is non-NULL. Allocating memory for directory...\n");

        //  non-root directory.
        // allocating memory for the directory here.
        Directory_Entry* new_directory = malloc(block_size * vcb->entries_per_dir);

        // If  couldn't get the memory, just print an error message and return -1.
        if (new_directory == NULL) {
            printf("[ LOAD DIRECTORY ] : Failed to allocate memory for directory.\n");
            return -1;
        }

        // reading the directory from the disk here.
        printf("[ LOAD DIRECTORY ] : Reading directory from disk...\n");
        LBAread(new_directory, 1, directory->dir_first_cluster);
        block_num =  new_directory->dir_first_cluster;
        free(new_directory);
    }
 
    printf("[ LOAD DIRECTORY ] : Directory loaded successfully.\n");
    return block_num;;
}


/*
 * This function clears the current working directory
 */
void clear_current_working_directory()
{
    // We're setting the path to an empty string and the cluster to -1.
    memset(current_working_directory->path, '\0', sizeof(current_working_directory->path));
    current_working_directory->cluster = -1;
}
