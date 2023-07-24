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
* File: root_init.h
*
* Description: File for handling init. of root directory
* contains structure for directory entries and functions for 
* handling the init. of root directory. 
**************************************************************/
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#ifndef _ROOT_INIT_H
#define _ROOT_INIT_H

#define NAME_MAX_LENGTH	11 //only support short names
#define DIRECTORY_MAX_LENGTH	64
#define MAX_PATH_LENGTH		255
#define IS_ACTIVE 	1<<27 // sixth bit of the dir_attr in DE will indicate whether in use or not
#define IS_DIR		1<<28 // fifth bit indicating whether DE is a directory	
#define DIRTY_DIR	1<<26 // bit to indicate whether a dir is empty or not
typedef struct Directory_Entry {
    char dir_name[NAME_MAX_LENGTH];
    char path[MAX_PATH_LENGTH];
    uint32_t dir_attr;
    uint32_t dir_first_cluster;
    uint32_t dir_file_size;
} Directory_Entry;

extern Directory_Entry* root_directory;
extern char * cwd;
extern Directory_Entry * current_directory;
/*
* This function initializes a new directory. 
* If a parent directory is provided, it creates a new directory under it. 
* If not, the directory is created in the root cluster.
*/
Directory_Entry * init_directory(uint64_t block_size, Directory_Entry* parent_directory, char* name);

/*
* This function resets the current working directory to its default state. 
* This includes setting the directory path to an empty string and the directory cluster to -1.
*/
void clear_current_working_directory();

/*
* This function loads a directory from the disk into memory.
* If the provided directory is NULL, it will load the root directory.
*/
int load_directory(uint64_t block_size, Directory_Entry* directory);

int load_root(void);

// help read a directory from disk
int read_from_disk(void * buffer, int start_block, int blocks_need, int block_size);

// help write a direcotry to disk
int write_to_disk(void * buffer, int start_block, int blocks_need, int block_size);


#endif // _ROOT_INIT_H
