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
Directory_Entry *current_directory = NULL;


int load_root(){
    int min_bytes_needed = vcb->entries_per_dir * sizeof(Directory_Entry);
    int blocks_need = (min_bytes_needed + vcb->bytes_per_block -1) / vcb->bytes_per_block;
    int malloc_bytes = blocks_need * vcb->bytes_per_block;
    
    int start_block = vcb->root_cluster;
    int block_size = vcb->bytes_per_block;
    root_directory = malloc(malloc_bytes);
        
    printf("[LOAD ROOT] start root block: %d\n", start_block);
    printf("[LOAD ROOT] numb of blocks for root: %d\n", blocks_need);
    int check = read_from_disk( (void *) root_directory, start_block, blocks_need, block_size);
    if (check == -1) {
	    printf("[LOAD ROOT] failed to load root\n");
	    return -1;
    }
    current_directory = root_directory;

    // root already loaded in memory using LBAread why loading it again?
   // load_directory (vcb->bytes_per_block, root_directory);
    cwd = malloc(NAME_MAX_LENGTH);
    cwd[0] = '/';
        cwd[1] = '\0';
	current_directory = root_directory;
    return 0;
}

Directory_Entry * init_directory(uint64_t block_size, Directory_Entry *parent, char *name) {
 	int min_bytes_needed = vcb->entries_per_dir * sizeof(Directory_Entry);
	int blocks_need = (min_bytes_needed + block_size -1) / block_size;
	int malloc_bytes = blocks_need * block_size;
	
	Directory_Entry * entries = malloc(malloc_bytes);


	for (int i = 2; i < vcb->entries_per_dir; i++) {
		// init entries
		strcpy(entries[i].dir_name, "entry");
	}

	if (parent == NULL) { // root here
		parent = entries;
	}
	
	if (strlen(name) + 1 + strlen(parent[0].path) > MAX_PATH_LENGTH){
		printf("path length exceed limit\n");
		return NULL;
	}

	// initialze the directory itself
	
	// concat the parenth path to the child's name
	// to make the child's path
	if ( strcmp(parent[0].path, "/") != 0) {
		strcpy(entries[0].path, parent[0].path);
	}
	strcat(entries[0].path, "/");
	strcat(entries[0].path, name);
	
	strcpy(entries[0].dir_name, ".");
	entries[0].dir_file_size = min_bytes_needed;
	entries[0].dir_first_cluster = allocate_blocks(blocks_need); // testing for root
	entries[0].dir_attr |= (IS_ACTIVE | IS_DIR);



	//
	//testing hex values
	//
	//
	
	printf("hex values of dir_name: %X\n",entries[0].dir_name[0]);
	printf("hex values of block location: %X\n", entries[0].dir_first_cluster);
	printf("hex values of file size: %X\n",entries[0].dir_file_size);
	printf("hex values of : %X\n",entries[0].dir_attr);
	//
	//
	//testing hex values
	//
	//
	//


	// link the second entry to the parent
	parent[0].dir_attr |= DIRTY_DIR;
	strcpy(entries[1].dir_name, "..");
	entries[1].dir_file_size = parent[0].dir_file_size;
	entries[1].dir_first_cluster = parent[0].dir_first_cluster;
	strcpy(entries[1].path, parent[0].path);
	entries[1].dir_attr = parent[0].dir_attr;

	// commit data to disk
	int start_block = entries[0].dir_first_cluster;
	int count_block = 0;
	printf("[INIT ROOT] root start block: %d\n", start_block);
	int check = write_to_disk( (void *) entries, start_block, blocks_need, block_size);
	
	if (check == -1) { //failed to write to disk
		return NULL;
	}
	return entries;

}


int read_from_disk(void * buffer, int start_block, int blocks_need, int block_size){
	int count_block = 0;
	int offset = 0;

	while (start_block != EOF_BLOCK && count_block != blocks_need) {
		if (LBAread(buffer + offset, 1, start_block) != 1){
			printf("failed to read from disk\n");
			return -1;
		}
		//start_block++;
		start_block = get_next_block(start_block);
		count_block++;
		offset += block_size;
	}

	return 0;
}
int write_to_disk(void * buffer, int start_block, int blocks_need, int block_size){
	int count_block = 0;
	int offset = 0;

	while (start_block != EOF_BLOCK && count_block != blocks_need) {
		if (LBAwrite(buffer + offset, 1, start_block) != 1){
			printf("%d\n", EOF_BLOCK);
			printf("failed to write to disk\n");
			return -1;
		}
		//start_block++;
		start_block = get_next_block(start_block); // testing to see if it write to 
		//the disk correctly first before fixing FAT

		count_block++;
		offset += block_size;
	}

	return 0;
}


