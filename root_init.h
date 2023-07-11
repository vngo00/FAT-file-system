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
#ifndef _ROOT_INIT_H
#define _ROOT_INIT_H

#define NAME_MAX_LENGTH	11 //only support short names
#define DIRECTORY_MAX_LENGTH	4096

typedef struct Directory_Entry {
	char dir_name[NAME_MAX_LENGTH];
	uint8_t dir_attr;
	uint16_t dir_first_cluster;
	uint32_t dir_file_size;	
} Directory_Entry;

typedef struct Current_Working_Directory {
	char path[DIRECTORY_MAX_LENGTH];
	uint32_t cluster;
} Current_Working_Directory;

Current_Working_Directory* current_working_directory;
Directory_Entry* root_directory;

uint32_t init_root_directory(uint64_t block_size);
int load_root_directory(uint64_t block_size);
void clear_current_working_directory();

#endif // _ROOT_INIT_H
