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
#include <assert.h> //for assert()

#include "fsLow.h"
#include "mfs.h"
#include "vcb_.h"
#include "FAT.h"
#include "root_init.h"

// Initialize the current working directory and root directory
Directory_Entry *root_directory = NULL;
char * cwd = NULL;


/*
 * Function to initialize a new directory.
 * Can also be used to create the root directory
 */
uint32_t init_directory(uint64_t block_size, Directory_Entry* parent_directory, char* dir_name) {
    assert(dir_name != NULL);

    if (strlen(dir_name) > NAME_MAX_LENGTH) {
        fprintf(stderr, "[ INIT DIRECTORY ]: Directory name is too long.\n");
        return -1;
    }

    Directory_Entry* new_directory = calloc(1, sizeof(Directory_Entry));
    if (new_directory == NULL) {
        perror("[ INIT DIRECTORY ]: Failed to allocate memory for new directory");
        return -1;
    }

    strncpy(new_directory->dir_name, dir_name, NAME_MAX_LENGTH);
    new_directory->dir_attr |= 1UL << 4;

    uint32_t first_cluster = parent_directory == NULL ? vcb->root_cluster : find_free_block();

    new_directory->dir_file_size = vcb->entries_per_dir * sizeof(Directory_Entry);
    new_directory->entries_array_location = find_free_block();
    new_directory->dir_first_cluster = first_cluster;

    if (parent_directory == NULL) {
        new_directory->path[0] = '/';
        cwd = calloc(NAME_MAX_LENGTH + 1, sizeof(char));
        cwd[0] = '/';
    }

    Directory_Entry* entries = calloc(vcb->entries_per_dir, sizeof(Directory_Entry));
    if (entries == NULL) {
        perror("[ INIT DIRECTORY ]: Failed to allocate memory for directory entries");
        free(new_directory);
        return -1;
    }

    Directory_Entry dot_entry = { .dir_name = ".", .dir_attr = new_directory->dir_attr, .dir_first_cluster = first_cluster, .dir_file_size = new_directory->dir_file_size, .entries_array_location = new_directory->entries_array_location };
    Directory_Entry dot_dot_entry = { .dir_name = "..", .dir_attr = new_directory->dir_attr, .dir_first_cluster = first_cluster, .dir_file_size = new_directory->dir_file_size, .entries_array_location = new_directory->entries_array_location };

    entries[0] = dot_entry;
    entries[1] = dot_dot_entry;

    if (parent_directory != NULL) {
        char buff[NAME_MAX_LENGTH + 1];
        strncpy(buff, parent_directory->path, NAME_MAX_LENGTH);
        strncat(buff, "/", NAME_MAX_LENGTH - strlen(buff));
        strncat(buff, dir_name, NAME_MAX_LENGTH - strlen(buff));

        if (add_entry_to_parent(parent_directory, new_directory, buff) == -1) {
            free(entries);
            free(new_directory);
            return -1;
        }
    }

    int ret_value = LBAwrite(new_directory, 1, new_directory->dir_first_cluster);
    if (ret_value == -1) {
        perror("[ INIT DIRECTORY ]: Error writing the new directory");
        free(new_directory);
        free(entries);
        return -1;
    }

    LBAwrite(entries, vcb->entries_per_dir, new_directory->entries_array_location);

    free(entries);
    free(new_directory);

    return first_cluster;
}

int add_entry_to_parent(Directory_Entry* parent_directory, Directory_Entry* new_directory, char* new_path) {
    assert(parent_directory != NULL);
    assert(new_directory != NULL);
    assert(new_path != NULL);

    Directory_Entry* parent_array = malloc(vcb->entries_per_dir * sizeof(Directory_Entry));
    if (parent_array == NULL) {
        perror("[ INIT DIRECTORY ]: Error allocating memory for parent array");
        return -1;
    }

    if (LBAread(parent_array, 1, parent_directory->entries_array_location) == -1) {
        perror("[ INIT DIRECTORY ]: Error reading parent directory entries");
        free(parent_array);
        return -1;
    }

    int i;
    for (i = 0; i < DIRECTORY_MAX_LENGTH; i++) {
        if (parent_array[i].dir_name[0] == '\0') {
            parent_array[i] = *new_directory;
            break;
        }
    }

    if (i == DIRECTORY_MAX_LENGTH) {
        fprintf(stderr, "[ INIT DIRECTORY ]: Parent directory has no space for new directory.\n");
        free(parent_array);
        return -1;
    }

    LBAwrite(parent_array, 1, parent_directory->entries_array_location);
    strncpy(new_directory->path, new_path, NAME_MAX_LENGTH);

    free(parent_array);
    return 0;
}

int load_root(){
    root_directory = malloc(vcb->bytes_per_block*vcb->entries_per_dir);
    LBAread(root_directory,1, vcb->root_cluster);
    load_directory (vcb->bytes_per_block, root_directory);
    cwd = malloc(NAME_MAX_LENGTH);
    cwd[0] = '/';
        cwd[1] = '\0';
    return 0;
}

/*
 * This function loads a directory from the disk into memory.
 * If the provided directory is NULL, it will load the root directory
 */
int load_directory(uint64_t block_size, Directory_Entry* directory) {
    printf("[ LOAD DIRECTORY ] : Starting process to load directory...\n");

    int block_num = -1;
    printf("[ LOAD DIRECTORY ] : Initialized block_num to -1...\n");

    if (directory == NULL) {
        printf("[ LOAD DIRECTORY ] : Detected NULL directory, assuming root directory...\n");

        root_directory = malloc(block_size * vcb->entries_per_dir);
        if (root_directory == NULL) {
            printf("[ LOAD DIRECTORY ] : Failed to allocate memory for root directory...\n");
            return -1;
        }

        printf("[ LOAD DIRECTORY ] : Reading root directory from disk at location %d...\n", vcb->root_cluster);
        LBAread(root_directory, 1, vcb->root_cluster);
        printf("[ LOAD DIRECTORY ] : Completed reading root directory from disk at location %d...\n", root_directory->dir_first_cluster);

        printf("[ LOAD DIRECTORY ] : Root directory loaded successfully...\n");
        return vcb->root_cluster;
    }
    else {
        printf("[ LOAD DIRECTORY ] : Detected non-NULL directory, starting process for non-root directory...\n");

        Directory_Entry* new_directory = malloc(block_size * vcb->entries_per_dir);
        if (new_directory == NULL) {
            printf("[ LOAD DIRECTORY ] : Failed to allocate memory for directory...\n");
            return -1;
        }

        printf("[ LOAD DIRECTORY ] : Reading directory from disk at location %d...\n", directory->dir_first_cluster);
        LBAread(new_directory, 1, directory->dir_first_cluster);
        printf("[ LOAD DIRECTORY ] : Completed reading directory from disk, directory name: %s...\n", new_directory->dir_name);

        block_num = new_directory->dir_first_cluster;
        printf("[ LOAD DIRECTORY ] : block_num set to %d...\n", block_num);

        printf("[ LOAD DIRECTORY ] : Freeing memory allocated for directory...\n");
        free(new_directory);
    }

    printf("[ LOAD DIRECTORY ] : Directory loaded successfully, returning block number: %d...\n", block_num);
    return block_num;
}


/*
 * This function clears the current working directory
 */
void clear_current_working_directory()
{
    if (cwd != NULL) {
        memset(cwd, 0, 255);
    }
}
