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
* File: mfs.c
*
* Description:
**************************************************************/
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "fsLow.h"
#include "vcb_.h"
#include "FAT.h"
#include "root_init.h"
#include "mfs.h"
#include "fsLow.h"

extern int entries_per_dir; // need to know the number of the entries per directory
extern int bytes_per_block;

int get_empty_entry(Directory_Entry * parent);
void free_dir(Directory_Entry *dir);
int is_dir(Directory_Entry entry);

// This function changes the current working directory to the specified path.
// It allows us to navigate to a different directory within the file system.
char *fs_getcwd(char *path, size_t size)
{

	//Uses the current_directory[0].path which is the full path for the cwd
    strncpy(path, current_directory[0].path, size);

	// adding code to test b_read, need to remove !!!
	if(fs_mkfile("g")==0){
		printf("[Test]: File was made.\n");	
	}
	int index = find_target_entry(current_directory,&"g");
	Directory_Entry *temp = get_target_directory(current_directory[index]);
	printf("[Test]: number of blocks in file %i .\n", temp->dir_file_size);	

    return path;
}

int fs_setcwd(char *path)
{
    parsed_entry entry;

	//First Check setup for parse_directory_path return value
	//0 Succeeds, -1 fails
    if ( parse_directory_path(path, &entry) == -1) {
		printf("[ FS SETCWD ]: Invalid path.\n");
        free_dir(entry.parent);
		return -1;
	}
    
	//Second Check setup for if entry values are correctly updated from parse path
	//Checks if the parent of the directory exists, will be itself if root dir
    if (entry.index == -1 || entry.parent == NULL) {
		printf("[ FS SETCWD ] Directory does not exist within current directory\n");
		free_dir(entry.parent);
		return -1;
	}

	//Changing directory requires the path to be a directory in order to set
	//The current directory to it
    if (!fs_isDir(path)){
        printf("[ FS SETCWD ]: Not a directory\n");
        free_dir(entry.parent);
        return -1;
    }

	//Saves previous current_directory, in case current_directory gets incorrectly assigned
	//Creates a temp save point before cwd is lost 
    Directory_Entry* temp = current_directory;

	//Gets new target directory through return value of get_target (a directory entry)
    Directory_Entry* target = get_target_directory(entry.parent[entry.index]);
    if (target == NULL) return -1; //Quick Check for target value existing

	//Actual Update and setting the new current directory
    current_directory = target;
    
    free_dir(temp);
   
    return 0;
}

/*
// This helper function checks whether the given attribute represents a directory.
// It returns 1 if the attribute indicates a directory, and 0 otherwise.
int check_directory_attribute(int attribute)
{
    return ((attribute >> 4) & 1);
}

*/

// This function searches for a directory entry that matches the provided token (directory name)
// within the given array of directory entries (current_dir_ent).
// It iterates through each directory entry in the current directory until a match is found.
// If a match is found, it allocates memory for a new directory entry, copies the matching entry into it,
// and returns the new directory entry. If no match is found, it returns NULL.
Directory_Entry *get_target_directory(Directory_Entry entry)
{
    // check if we want root or current dir
    if (strcmp(entry.path, "/") == 0)
        return root_directory;
    if (strcmp(entry.path, current_directory[0].path) == 0)
        return current_directory;

    // if not load it to memory

	//Block_size and blocks_need are used to forumalate amount of memory needed
	//for a the target directory to have
    int block_size = bytes_per_block; 
    int blocks_need = (entry.dir_file_size + block_size - 1) / block_size;
    Directory_Entry *ret = malloc(blocks_need * block_size);

	//Runs through read Disk which runs through the fat implementation of getNew
	//Returns of successful or failure (-0,-1)
    if (read_from_disk(ret, entry.dir_first_cluster, blocks_need, block_size) == -1)
    {
        printf("[LOAD DIR] can't load dir\n");
        return NULL;
    }
    return ret;
}

int find_target_entry(Directory_Entry *current_dir_ent, char *token)
{

    for (int i = 0; i < entries_per_dir; i++)
    {
        //printf("[ ind_target_dir] : Checking entry number: %d, name: %s, attribute: %d\n", i, current_dir_ent[i].dir_name, current_dir_ent[i].dir_attr);

        if (strcmp(current_dir_ent[i].dir_name, token) == 0)
        {
            //printf("[ ind_target_dir ] : Found matching directory entry at index: %d\n", i);
            return i;
        }
    }
    //printf("[ ind_target_dir ] : No matching directory entry found. Returning NULL.\n");
    return -1;
}


int parse_directory_path(char *path, parsed_entry *entry) {
	if (path == NULL) return -1;
	
	Directory_Entry *start_dir;
	// checking for starting point
	if ( path[0] != '/') { //relative
		start_dir = current_directory;
	} else {
		start_dir = root_directory;
	}

	Directory_Entry * parent = start_dir;

	int index = -1;
	char * token = strtok(path, "/");

	// either it is current dir or root
	if (token == NULL) {
		entry->parent = start_dir;
		entry->index = 0;
		return 0;
	}

	while (token != NULL) {
		char * token2 = strtok(NULL, "/");

		if (token2 == NULL) {
			index = find_target_entry(parent, token);
			entry->name = token;
		} else {
			index = find_target_entry(parent, token);
			if (index == -1) return -1;
			if (!is_dir(parent[index])) return -1;
			Directory_Entry *temp = get_target_directory(parent[index]);
			if (parent != start_dir) free_dir(parent);
			parent = temp;
		}
		token = token2;
	}
	entry->parent = parent;
	entry->index = index;
	return 0;
}

/*
 *help function to find empty entry
 *
 */
int get_empty_entry(Directory_Entry * parent) {
	for (int i = 2; i < entries_per_dir; i++) {
		if (!(parent[i].dir_attr & IS_ACTIVE))
			return i;
	}
	return -1;
}

int fs_mkdir(const char *pathname, mode_t mode)
{
	parsed_entry entry;
	if ( parse_directory_path(pathname, &entry) == -1) {
		printf(" [MKDIR] invalid path\n");
		return -1;
	}

	if ( strcmp(entry.name, "") == 0  || entry.index != -1 || entry.parent == NULL){
		free_dir(entry.parent);
		printf("[MKDRI] error\n");
		return -1;
	}
	
	int ret = 0;

	Directory_Entry *child = init_directory(bytes_per_block, entry.parent, entry.name);
	int index = get_empty_entry(entry.parent);

	strncpy(entry.parent[index].dir_name, entry.name, NAME_MAX_LENGTH);
	strcpy(entry.parent[index].path, child[0].path);
	entry.parent[index].dir_file_size = child[0].dir_file_size;
	entry.parent[index].dir_first_cluster = child[0].dir_first_cluster;
	entry.parent[index].dir_attr = child[0].dir_attr;

	// commit new data to disk
	int block_size = bytes_per_block;
	int blocks_need = (entry.parent[0].dir_file_size + block_size -1) / block_size;
	if ( write_to_disk(
			entry.parent,
			entry.parent[0].dir_first_cluster,
			blocks_need,
			block_size) == -1) {
		printf("[MKDIR] failed to write to disk\n");
		ret = -1;
	}
	free_dir(entry.parent);
	free_dir(child);
	return ret;


}


int fs_rmdir(const char *pathname) {


	parsed_entry entry;
	if (parse_directory_path(strdup(pathname), &entry) == -1) {
		printf("[RMDIR] invalid path\n");
		free_dir(entry.parent);
		return -1;
	}

	if (entry.index == -1 || entry.parent == NULL) {
		printf("[RMDIR] dir not exist\n");
		free_dir(entry.parent);
		return -1;
	}
	if (fs_isFile(strdup(pathname))) {
		free_dir(entry.parent);
		printf("[RMDIR] file\n");
		return -1;
	}
	if (strcmp(entry.name, "") == 0 || strcmp(entry.name, "/") == 0) {
		printf("[RMDIR] get lost\n");
		free_dir(entry.parent);
		return -1;
	}

	if (entry.parent[entry.index].dir_attr & DIRTY_DIR) {
		printf("[RMDIR] not empty dir\n");
		free_dir(entry.parent);
		return -1;
	}

	// load child
	Directory_Entry * child = get_target_directory(entry.parent[entry.index]);
	if ( child == NULL) {
		printf("[RMDIR] NULL CHILD ?\n");
		free_dir(entry.parent);
		return -1;
	}

	child[1].dir_first_cluster = -1; // unlink the .. entry that links to the parent

	int block_size = bytes_per_block;
	int bytes_need = child[0].dir_file_size;
	int blocks_need = (block_size + bytes_need -1 ) / block_size;
	int child_start = child[0].dir_first_cluster;
	if (write_to_disk(child, child_start, blocks_need, block_size) == -1 ) {
		printf("Can't write to disk\n");
		free_dir(entry.parent);
		free_dir(child);
		return -1;
	}

	free_dir(child);

	release_blocks(child_start);

	strcpy(entry.parent[entry.index].dir_name, "entry");
	strcpy(entry.parent[entry.index].path, "");
	entry.parent[entry.index].dir_first_cluster = -1;
	entry.parent[entry.index].dir_file_size = 0;
	entry.parent[entry.index].dir_attr = 0;

	if (write_to_disk(entry.parent, entry.parent[0].dir_first_cluster, blocks_need, block_size) == -1) {
		printf("can't write to disk\n");
		free_dir(entry.parent);
		return -1;
	}

	free_dir(entry.parent);
	
	return 0;
}





// Function to check if the given filename corresponds to a regular file
int fs_isFile(char *filename)
{
	int ret = fs_isDir(filename);
	if (ret == -1) return ret;
	return !ret;
}

// Function to check if the given pathname corresponds to a directory or not
int fs_isDir(char *pathname)
{
	parsed_entry entry;
	if ( parse_directory_path(pathname, &entry) == -1) {
		printf("[IS DIR] invalid path\n");
		return -1;
	}

	if (entry.index == -1 || entry.parent == NULL) {
		printf("[IS DIR] %s does not exist\n", entry.name);
		free_dir(entry.parent);
		return -1;
	}

	int ret = is_dir(entry.parent[entry.index]);
	free_dir(entry.parent);
	return ret;
 
}





int fs_mkfile(char *filename) {

	parsed_entry entry;
	if (parse_directory_path(strdup(filename), &entry) == -1) {
		printf("[MKFILE] invalid path\n");
		return -1;
	}

	if (strcmp(entry.name, "") == 0) {
		printf("[MKFILE] name ?\n");
		free_dir(entry.parent);
		return -1;
	}

	if (entry.index != -1 || entry.parent == NULL) {
		printf("[MKFILE] %s already exists\n", entry.name);
		free_dir(entry.parent);
		return -1;
	}

	int index = get_empty_entry(entry.parent);
	strncpy(entry.parent[index].dir_name, entry.name, NAME_MAX_LENGTH);
	int blocks = 1;
	entry.parent[index].dir_first_cluster = allocate_blocks(blocks);
	entry.parent[index].dir_attr |= IS_ACTIVE;
	printf("[MKFILE] file location: %d\n", entry.parent[index].dir_first_cluster);

	// commit new data to disk
	int block_size = bytes_per_block;
	int blocks_need = (entry.parent[0].dir_file_size + block_size -1) / block_size;
	if (write_to_disk(entry.parent, entry.parent[0].dir_first_cluster, blocks_need, block_size) == -1) {
		printf("[MKFILE] failed to make file\n");
		free_dir(entry.parent);
		return -1;
	}
	
	free_dir(entry.parent);


	return 0;
}


int fs_mvFile(char *filename, char *pathname) {

	if ( fs_isDir(filename) == 1) {
		return -1;
	}

	parsed_entry source;
	if ( parse_directory_path (filename, &source) == -1) {
		printf("[MVFILE] invalid path\n");
		return -1;
	}

	if ( source.index == -1 || source.parent == NULL) {
		printf("[MVILFE] %s does not exist\n", source.name);
		free_dir(source.parent);
		return -1;
	}

	if (fs_isFile(pathname) == 1) {
		printf(" [MVILFE] a file\n");
		return -1;
	}

	parsed_entry destination;
	if (parse_directory_path(pathname, &destination) == -1) {
		printf("[MVFILE] invalid path\n");
		free_dir(destination.parent);
		return -1;
	}

	if (destination.index == -1 || destination.parent == NULL) {
		free_dir(destination.parent);
		printf("[MFILE] dir does not exists\n");
		return -1;
	}

	Directory_Entry * dest_dir = get_target_directory(destination.parent[destination.index]);
	if ( strcmp(dest_dir[0].path, source.parent[0].path == 0) ) { // same dir
		printf("[MVFILE] same dir\n");
		free_dir(source.parent);
		free_dir(destination.parent);
		free_dir(dest_dir);
	}

	int index = get_empty_entry(dest_dir);
	int block_size = bytes_per_block;
	int blocks_need = (dest_dir[0].dir_file_size + block_size - 1) / block_size;

	// start the moving process
	strcpy(dest_dir[index].dir_name, source.name);
	dest_dir[index].dir_file_size = source.parent[source.index].dir_file_size;
	dest_dir[index].dir_first_cluster = source.parent[source.index].dir_first_cluster;
	dest_dir[index].dir_attr = source.parent[source.index].dir_attr;

	free_dir(source.parent);
	free_dir(destination.parent);

	if (write_to_disk(dest_dir, dest_dir[0].dir_first_cluster, blocks_need, block_size) == -1) {
		printf("[MVFILE] failed to write to disk\n");
		return -1;
	}

	free_dir(dest_dir);


	return 0;

}






int fs_delete(char *filename)
{

    // if not file do no delet
    if (fs_isDir(strdup(filename)) == 1)
    {
        printf("Can't delete a directory\n");
	return -1;
    }
    
    // grab the directory entry of the file
    parsed_entry entry;
    if (parse_directory_path(strdup(filename), &entry) == -1) {
	    return -1;
    }

    if (entry.index == -1 || entry.parent == NULL){
	free_dir(entry.parent);
        printf("not a valid file\n");
        return -1;
    }

    // free the blocks
    release_blocks(entry.parent[entry.index].dir_first_cluster);


    // need to clear out the metadata of the file from the directory entry;
    strcpy(entry.parent[entry.index].dir_name, "entry");
    strcpy(entry.parent[entry.index].path, "");
    entry.parent[entry.index].dir_attr = 0;
    entry.parent[entry.index].dir_first_cluster = 0;
    entry.parent[entry.index].dir_file_size = 0;
    
    // update data to disk 
    int bytes_need = entry.parent[0].dir_file_size;
    int blocks_need = (bytes_need + bytes_per_block -1) / bytes_per_block;
    if (write_to_disk(entry.parent,
			    entry.parent[0].dir_first_cluster,
			    blocks_need, bytes_per_block) == -1) {
	    printf("[FS DELETE] can't write to disk\n");
	    free_dir(entry.parent);
	    return -1;
    }

    // free directory if not root or current dir
    free_dir(entry.parent);


    return 0;
}

/*
 *
 *
 * Directory iteration functions
 *
 */
//
// return directory descriptor which will keep the information
// of a target directory
fdDir *fs_opendir(const char *pathname)
{
    // check if pathname is a directory or a file
    if (fs_isFile(strdup(pathname)) == 1)
    {
        printf("not a directory\n");
        return NULL;
    }
    
    parsed_entry entry;
    if (parse_directory_path(strdup(pathname), &entry) == -1) {
        printf ("invalid pathname\n");
        return NULL;
    }

    if (entry.index == -1 || entry.parent == NULL) {
	    printf("[OPEN DIR] dir not exists\n");
	    free_dir(entry.parent);
	    return NULL;
    }

    Directory_Entry *child = get_target_directory(entry.parent[entry.index]);
    if (child == NULL) {
	    printf("[OPEN DIR] can;t bring to mem\n");
	    free_dir(entry.parent);
	    return NULL;
    }

    fdDir * dir = malloc(sizeof(fdDir));
    dir->d_reclen = entries_per_dir; // the total number of entries;
    dir->dirEntryPosition = 0;			// current position of entry in directory
    dir->directory = child;		// target directory that the caller wants;
    dir->di = malloc(sizeof(struct fs_diriteminfo));


    if (entry.parent != child){
	    free_dir(entry.parent);
    } 

    return dir;

    
}

/*
 * helper functions for readdir
 */
int is_used(Directory_Entry entry)
{
    return entry.dir_attr & IS_ACTIVE;
}
int is_dir(Directory_Entry entry)
{
    return entry.dir_attr & IS_DIR;
}

struct fs_diriteminfo *fs_readdir(fdDir *dirp)
{
    if (dirp == NULL)
        return NULL;

    for (int i = dirp->dirEntryPosition; i < dirp->d_reclen; i++)
    {
        dirp->dirEntryPosition++;
        if (is_used(dirp->directory[i]))
        {
            strncpy(dirp->di->d_name, dirp->directory[i].dir_name, 256);
            if (is_dir(dirp->directory[i]))
                dirp->di->fileType = DT_DIR;
            else
                dirp->di->fileType = DT_REG;

            return dirp->di;
        }
    }
    return NULL;
}

int fs_closedir(fdDir *dirp)
{
    if (dirp == NULL)
    {
        printf("dirp is null\n");
        return -1;
    }
    free_dir(dirp->directory);
    free(dirp->di);
    dirp->di = NULL;
    dirp = NULL;

    return 0;
}

int fs_stat(const char *path, struct fs_stat *buf)
{
	parsed_entry entry;
	if (parse_directory_path(strdup(path), &entry) == -1) {
		printf("[FS STAT] invalid path\n");
		return -1;
	}
	if (entry.index == -1 || entry.parent == NULL) {
		free_dir(entry.parent);
		printf("[FS STAT] %s does not exists", entry.name);
		return -1;
	}

	buf->st_size = entry.parent[entry.index].dir_file_size;

	int block_size = bytes_per_block;
	int bytes_need = entry.parent[entry.index].dir_file_size;
	int blocks_need = (bytes_need + block_size - 1) / block_size;
	buf->st_blksize = block_size;
	buf->st_blocks = blocks_need;
	
	return 0;
	
}


void free_dir(Directory_Entry * dir) {
	if (dir != current_directory && dir != root_directory) {
		free(dir);
		dir == NULL;
	}
}

int fs_renameDirectoryOrFile(const char *path, const *newName)
{
	parsed_entry entry;

	//Check for if name exists as an already created file or directory
	//Looks to see if the given path in newName is file/dir which would return 0
	if (fs_isFile(newName) == 0 || fs_isDir(newName) == 0 ){
			printf("[ FS RENAME ]: Name already exists.\n");
			return -1;
    }

	//Changes print statement based on whether it is a dir or file
	if (fs_isDir(path)){
        printf("[ FS RENAME ]: Changing dir name of %s to %s\n", path, newName);
    }
	else{
		printf("[ FS RENAME ]: Changing file name of %s to %s\n", path, newName);
	}

	//Parses through path to build the path and send it over to entry struct
    if ( parse_directory_path(path, &entry) == -1) {
		printf("[ FS RENAME ]: Invalid path.\n");
        free_dir(entry.parent);
		return -1;
	}

	//Need to get the current Directory entry that you are changing the name for
	//Entry struct containers the parent with with the correct index from running
	//the parse_directory_path function
    Directory_Entry* target = get_target_directory(entry.parent[entry.index]);

	//Needs to copy the user inputted name into the directory or file after correct checks
	strcpy(entry.parent[entry.index].dir_name, newName);

	//Immeditate clean of the entry after copying over
	free_dir(entry.parent);
    return 0;

}


