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
uint32_t init_directory(uint64_t block_size, Directory_Entry *parent_directory, char *dir_name)
{

    Directory_Entry *new_directory = malloc(sizeof(Directory_Entry));
    // Check for memory allocation
    if (new_directory == NULL)
    {
        printf("Failed to allocate memory for new directory");
        return -1;
    }

    strncpy(new_directory->dir_name, dir_name, strlen(dir_name));
    new_directory->dir_attr = new_directory->dir_attr |= 1UL << 4;
    /*
     * If parent directory in NULL, use the root cluster
     * else find the first free block
     */
    uint32_t first_cluster = parent_directory == NULL ? vcb->root_cluster : find_free_block();
    new_directory->dir_file_size = vcb->entries_per_dir * sizeof(Directory_Entry);
    new_directory->entries_array_location = find_free_block();
    new_directory->dir_first_cluster = first_cluster;
    // Allocate and initialize the entries array
    Directory_Entry *entries = malloc(vcb->entries_per_dir * sizeof(Directory_Entry));
    if (entries == NULL)
    {
        free(new_directory);
        return -1;
    }
    // Mark all directory entries as free
    for (int i = 2; i < vcb->entries_per_dir; i++)
    {
        entries[i].dir_name[0] = '\0';
    }

    Directory_Entry dot_entry;
    dot_entry.dir_name[0] = '.';
    dot_entry.dir_name[1] = '\0'; // Null-terminate the string
    dot_entry.dir_attr = new_directory->dir_attr;
    dot_entry.dir_first_cluster = first_cluster;
    dot_entry.dir_file_size = new_directory->dir_file_size;
    dot_entry.entries_array_location = new_directory->entries_array_location;

    // Create the ".." entry
    Directory_Entry dot_dot_entry;
    dot_dot_entry.dir_name[0] = '.';
    dot_dot_entry.dir_name[1] = '.';
    dot_dot_entry.dir_name[2] = '\0'; // Null-terminate the string
    dot_dot_entry.dir_attr = new_directory->dir_attr;
    dot_dot_entry.dir_file_size = new_directory->dir_file_size;
    dot_dot_entry.entries_array_location = new_directory->entries_array_location;

    entries[0] = dot_entry;
    entries[1] = dot_dot_entry;
    /*
     * If there is a parent directory, the first cluster of ".." should be the first cluster of the parent directory
     * If there is no parent directory (i.e., we are creating the root directory), the first cluster of ".." should
     * be the first cluster of the new directory itself
     */
    if (parent_directory == NULL)
    {
        dot_dot_entry.dir_first_cluster = first_cluster;
    }
    else
    {
        dot_dot_entry.dir_first_cluster = parent_directory->dir_first_cluster;
        Directory_Entry *parent_array = malloc(vcb->entries_per_dir * sizeof(Directory_Entry));
        LBAread(parent_array, (sizeof(parent_array) + vcb->bytes_per_block - 1) / vcb->bytes_per_block, parent_directory->entries_array_location);
        int i = 0;
        for (; i < DIRECTORY_MAX_LENGTH; i++)
        {
            if (strlen(parent_array[i].dir_name) == 0)
            {
                parent_array[i] = *new_directory;
                break;
            }
        }
        if (i == DIRECTORY_MAX_LENGTH)
            printf("parent directory has no space for new directory.");
        return -1;
        LBAwrite(parent_array, (sizeof(Directory_Entry) * (DIRECTORY_MAX_LENGTH) + vcb->bytes_per_block - 1) / vcb->bytes_per_block, parent_directory->dir_first_cluster);
    }
    // Write the new directory to disk
    uint32_t ret_value = LBAwrite(new_directory, (sizeof(Directory_Entry) + vcb->bytes_per_block - 1) / vcb->bytes_per_block, new_directory->dir_first_cluster);
    printf("New Dir First Cluster %ld", new_directory->dir_first_cluster);
    if (ret_value == -1)
    {
        free(new_directory);
        new_directory = NULL;
        printf("Something went wrong when writing the new directory");
        return ret_value;
    }

    LBAwrite(entries, vcb->entries_per_dir * sizeof(Directory_Entry), new_directory->entries_array_location);

    return new_directory->dir_first_cluster;
}

/*
 * This function loads a directory from the disk into memory.
 * If the provided directory is NULL, it will load the root directory
 */
int load_directory(uint64_t block_size, Directory_Entry *directory)
{
    // Handle root directory or non-root directory
    if (directory == NULL)
    {

        root_directory = malloc(block_size * vcb->entries_per_dir);
        // Check for memory allocation
        if (root_directory == NULL)
        {
            printf("Failed to allocate memory for root directory");
            return -1;
        }

        LBAread(root_directory, 1, vcb->root_cluster);
    }
    else
    {

        Directory_Entry *new_directory = malloc(block_size * vcb->entries_per_dir);
        // Check for memory allocation
        if (new_directory == NULL)
        {
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
void clear_current_working_directory()
{
    // We're setting the path to an empty string and the cluster to -1.
    memset(current_working_directory->path, '\0', sizeof(current_working_directory->path));
    current_working_directory->cluster = -1;
}
