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

// This function builds an absolute path from the current directory and the provided pathname.
char* build_absolute_path(const char *pathname) {
    static char buf[255];

    printf("[ BUILD ABSOLUTE PATH ] : Building absolute path...\n");

    if (!pathname) {
        printf("[ BUILD ABSOLUTE PATH ] : NULL argument detected.\n");
        return NULL;
    }

    printf("[ BUILD ABSOLUTE PATH ] : Clearing buffer memory...\n");
    memset(buf, '\0', sizeof (buf));
    printf("[ BUILD ABSOLUTE PATH ] : Buffer after clearing: '%s'\n", buf);

    if (pathname[0] != '/') {
        printf("[ BUILD ABSOLUTE PATH ] : Path does not start with '/'. Concatenating paths...\n");
        if (!cwd) {
            printf("[ BUILD ABSOLUTE PATH ] : Current working directory is NULL.\n");
            return NULL;
        }
        printf("[ BUILD ABSOLUTE PATH ] : Current working directory: '%s'\n", cwd);
        strncpy(buf, cwd, sizeof (buf) - 1);
        buf[sizeof (buf) - 1] = '\0'; 
        printf("[ BUILD ABSOLUTE PATH ] : Buffer after concatenating current working directory: '%s'\n", buf);

        if (strcmp(cwd, "/") != 0) {
            strncat(buf, "/", sizeof (buf) - strlen(buf) - 1);
            buf[sizeof (buf) - 1] = '\0';
            printf("[ BUILD ABSOLUTE PATH ] : Buffer after adding '/': '%s'\n", buf);
        }

        printf("[ BUILD ABSOLUTE PATH ] : Pathname: '%s'\n", pathname);
        strncat(buf, pathname, sizeof (buf) - strlen(buf) - 1);
        buf[sizeof (buf) - 1] = '\0'; 
        printf("[ BUILD ABSOLUTE PATH ] : Buffer after concatenating pathname: '%s'\n", buf);
    } else {
        printf("[ BUILD ABSOLUTE PATH ] : Path starts with '/'. Copying pathname...\n");
        printf("[ BUILD ABSOLUTE PATH ] : Pathname: '%s'\n", pathname);
        strncpy(buf, pathname, sizeof (buf) - 1);
        buf[sizeof (buf) - 1] = '\0'; 
        printf("[ BUILD ABSOLUTE PATH ] : Buffer after copying pathname: '%s'\n", buf);
    }

    return buf;
}
// This function changes the current working directory to the specified path.
// It allows us to navigate to a different directory within the file system.
char* fs_getcwd(char * path, size_t size) {

    strncpy(path, current_directory[0].path, size);

    return path;
}

int fs_setcwd(char *path) {
    char *new_cwd = NULL;

    if (strcmp(path, "..") == 0) {
        printf("[ FS SETCWD ]: Handling parent directory case.\n");
        if (strcmp(cwd, "/") == 0) {
            printf("[ FS SETCWD ]: Already at root, cannot go up any further.\n");
            return 0;
        }
        new_cwd = malloc(255);
        if (new_cwd == NULL) {
            printf("[ FS SETCWD ]: Failed to allocate memory for new_cwd.\n");
            return -1;
        }
        strcpy(new_cwd, cwd);
        char *last_slash = strrchr(new_cwd, '/');
        if (last_slash != new_cwd) {
            *last_slash = '\0';
            printf("[ FS SETCWD ]: Moved up to parent directory.\n");
        } else {
            *(last_slash + 1) = '\0';
            printf("[ FS SETCWD ]: Moved up to root directory.\n");
        }
    } else {
        printf("[ FS SETCWD ]: Building new path.\n");
        new_cwd = malloc(255);
        if (new_cwd == NULL) {
            printf("[ FS SETCWD ]: Failed to allocate memory for new_cwd.\n");
            return -1;
        }
        strcpy(new_cwd, build_absolute_path(path));
    }

    if (cwd != NULL) { 
        free(cwd);
        printf("[ FS SETCWD ]: Freed old cwd.\n");
    }
    cwd = new_cwd;
    printf("[ FS SETCWD ]: Set cwd to new_cwd.\n");

    return 0;
}



// This helper function checks whether the given attribute represents a directory.
// It returns 1 if the attribute indicates a directory, and 0 otherwise.
int check_directory_attribute(int attribute) {
    return ((attribute >> 4) & 1);
}


// This function searches for a directory entry that matches the provided token (directory name)
// within the given array of directory entries (current_dir_ent).
// It iterates through each directory entry in the current directory until a match is found.
// If a match is found, it allocates memory for a new directory entry, copies the matching entry into it,
// and returns the new directory entry. If no match is found, it returns NULL.
Directory_Entry * get_target_directory(Directory_Entry entry) {
    // check if we want root or current dir
    if (entry.path == '/') return root_directory;
    if (strcmp(entry.path, current_directory[0].path) == 0)
	    return current_directory;
	
    // if not load it to memory
    int block_size = bytes_per_block;
    int blocks_need = (entry.dir_file_size + block_size -1) / block_size;
    Directory_Entry *ret = malloc(blocks_need * block_size);
    if (read_from_disk(ret, entry.dir_first_cluster, blocks_need, block_size) == -1){
	    printf("[LOAD DIR] can't load dir\n");
	    return NULL;
    }
    return ret;
}

// This function parses a directory path and finds the corresponding directory entry.
// It takes a path and a target parameter as input.
// If the provided path is NULL, the function prints an error message and returns NULL.
// Otherwise, it searches for the directory entry that matches the path.
// If a matching directory entry is found, it returns a pointer to the directory entry.
// If no matching directory entry is found or an error occurs, it returns NULL.
Directory_Entry* parse_directory_path(const char* path) {
    printf("[ PARSE DIRECTORY PATH ] : Parsing directory path, path: %s\n", path);
    if (!path) {
        printf("[ PARSE DIRECTORY PATH ] : NULL path detected.\n");
        return NULL;
    }
    if (strcmp(path, "/") == 0) {
        printf("[ PARSE DIRECTORY PATH ] : Root directory detected.\n");

        Directory_Entry* root_copy = malloc(sizeof (Directory_Entry));
        if (!root_copy) {
            printf("[ PARSE DIRECTORY PATH ] : Failed to allocate memory.\n");
            return NULL;
        }
        LBAread(root_copy, 1, vcb->root_cluster);
        return root_copy;
    }
    
    Directory_Entry* current_dir_ent = malloc(sizeof (Directory_Entry));
    char* temp = malloc(strlen(path) + 1);
    if (!current_dir_ent || !temp) {
        printf("[ PARSE DIRECTORY PATH ] : Failed to allocate memory.\n");
        free(current_dir_ent);
        free(temp);
        return NULL;
    }

    printf("[ PARSE DIRECTORY PATH ] : Copying root directory and path...\n");
    memcpy(current_dir_ent, root_directory, 512);
    strcpy(temp, path);

    printf("[ PARSE DIRECTORY PATH ] : Starting tokenization...\n");
    char* token = strtok(temp, "/");
    Directory_Entry* retVal = NULL;

    while (token != NULL) {
	   
	//
	////
	//// fix retVal
	//

	    
        retVal =NULL;// get_target_directory(current_dir_ent, token);
        if (!retVal) {
            printf("[ PARSE DIRECTORY PATH ] : No match found. Returning NULL.\n");
            break;
        }
        token = strtok(NULL, "/");
        current_dir_ent = retVal;
    }

    free(temp);
    return retVal;
}

int fs_mkdir(const char *pathname, mode_t mode) {
    printf("[ FS MKDIR ] : Attempting to create a directory with pathname: %s and mode: %d\n", pathname, mode);

    int block_num = 0;

    printf("[ FS MKDIR ] : Loading root directory...\n");
    int load_directory_result = load_directory(vcb->bytes_per_block, root_directory);

    if (load_directory_result == -1) {
        printf("[ FS MKDIR ] : Failed to load root directory. Load directory returned: %d.\n", load_directory_result);
        return -1;
    }

    printf("[ FS MKDIR ] : Root directory loaded successfully. Loaded directory result: %d\n", load_directory_result);

    Directory_Entry *new_directory = malloc(512);
    printf("[ FS MKDIR ] : Allocating memory for new directory...\n");

    if (new_directory == NULL) {
        printf("[ FS MKDIR ] : Failed to allocate memory for new directory.\n");
        return -1;
    }

    printf("[ FS MKDIR ] : Memory for new directory allocated successfully.\n");

    LBAread(new_directory, 1, load_directory_result);
    printf("[ FS MKDIR ] : Read new directory data from the block.\n");
    printf("[ FS MKDIR ] : Attempting to initialize new directory under root directory...\n");
    int init_directory_result = init_directory(vcb->bytes_per_block, new_directory, (char*) pathname);
    if (init_directory_result == -1) {
        printf("[ FS MKDIR ] : Failed to initialize directory. Init directory returned: %d.\n", init_directory_result);
        free(new_directory);
        return -1;
    }

    printf("[ FS MKDIR ] : Directory %s initialized successfully. Init directory result: %d\n", pathname, init_directory_result);

    free(new_directory);

    printf("[ FS MKDIR ] : Directory %s created successfully.\n", pathname);
    return 0;
}


// Function to check if the given filename corresponds to a regular file
int fs_isFile(char *filename) {
    // Parse the directory path to get the directory entry corresponding to the filename
    Directory_Entry* directory_entry = parse_directory_path(filename);

    // If the directory entry is NULL, it means the file was not found
    if (directory_entry == NULL) {
        // Print a message stating that the file was not found
        printf("[ FS ISFILE ] : File '%s' not found.\n", filename);
        // Return 0 indicating that the file does not exist
        return 0;
    }

    // Check if the directory attribute of the entry is not a directory
    // If returns 1, it's a file, otherwise it's a directory
    int is_file = !(directory_entry->dir_attr & IS_DIR);
    if (is_file) {
        // print a message stating that it's a regular file
        printf("[ FS ISFILE ] : File '%s' is a regular file.\n", filename);
    } else {
        // print a message stating that it's not a regular file
        printf("[ FS ISFILE ] : File '%s' is not a regular file.\n", filename);
    }
    // Deallocate the memory allocated to the directory entry to avoid memory leaks
    free(directory_entry);
    // Returns the status of whether the entity is a file or not
    return is_file;
}

// Function to check if the given pathname corresponds to a directory or not
int fs_isDir(char *pathname) {
    // Parse the directory path to get the directory entry corresponding to the pathname
    Directory_Entry* directory_entry = parse_directory_path(pathname);
    // If the directory entry is NULL, it means the directory was not found
    if (directory_entry == NULL) {
        // Print a message stating that the directory was not found
        printf("[ FS ISDIR ] : Directory '%s' not found.\n", pathname);
        // Returns 0 indicating that the directory does not exist
        return 0;
    }

    // Check if the directory attribute of the entry is a directory
    // If the result is non-zero, it's a directory
    int is_dir = directory_entry->dir_attr & IS_DIR;
    if (is_dir) {
        // prints a message stating that it's a directory
        printf("[ FS ISDIR ] : Directory '%s' is a directory.\n", pathname);
    } else {
        // prints a message stating that it's not a directory
        printf("[ FS ISDIR ] : Directory '%s' is not a directory.\n", pathname);
    }
    // Deallocate the memory allocated to the directory entry to avoid memory leaks
    free(directory_entry);
    // A non-zero result indicates it's a directory
    return is_dir != 0;
}



int fs_delete(char* filename) {
	
	// if not file do no delet
	if (fs_isDir(filename) == 1){
		printf("Can't delete a directory\n");
	}
	/*
	// grab the directory entry of the file
	parsed_entry entry;
	parse_directory_path(filename, &entry);
	
	if (entry.index == -1){
		printf("not a valid file\n");
		return -1;
	}

	// free the blocks
	release_blocks(entry.parent[entry.index]->dir_first_cluster);
	
	// need to clear out the metadata of the file from the directory entry;

	entry.parent[entry.index]->dir_name ="\0";
	entry.parent[entry.index]->path = "\0";
	entry.parent[entry.index]->dir_attr = 0;
	entry.parent[entry.index]->dir_first_cluster = 0;
	entry.parent[entry.index]->dir_file_size = 0;
	entry.parent[entry.index]->entries_array_location = 0;
	*/

	// need parse path to be done fisr
	return 0;	
}



/*
 *
 *
 * Directory iteration functions
 *
 */
//
//return directory descriptor which will keep the information
//of a target directory
fdDir * fs_opendir(const char *pathname) {
	// check if pathname is a directory or a file
	if (fs_isFile(pathname) == 1) {
		printf("not a directory\n");
		return NULL;
	}
	/*
	parsed_entry entry;
	if (parse_directory_path(pathname, &entry) == -1) {
		printf ("invalid pathname\n");
		return NULL;
	}
	
	fdDir * dir = malloc(sizeof(fdDir));
	dir->d_reclen = entrie_per_dir; // the total number of entries;
	dir->dirEntryPosition = 0;			// current position of entry in directory  
	dir->directory = entry.parent[index];		// target directory that the caller wants;
	dir->di = malloc(sizeof(struct fs_diriteminfo));
	return dir;

	*/
	return NULL;
}


/*
 * helper functions for readdir
 */
int isUsed(Directory_Entry entry) {
	return entry.dir_attr & IS_ACTIVE;
}
int isDir(Directory_Entry entry) {
	return entry.dir_attr & IS_DIR;
}

struct fs_diriteminfo *fs_readdir(fdDir *dirp) {
	if (dirp == NULL) return NULL;

	for (int i = dirp->dirEntryPosition; i < dirp->d_reclen; i++) {
		dirp->dirEntryPosition++;
		if (isUsed(dirp->directory[i])) {
			strcpy(dirp->directory[i].dir_name, dirp->di->d_name);
			if(isDir(dirp->directory[i]))
				dirp->di->fileType = DT_DIR;
			else
				dirp->di->fileType = DT_REG;	

		return dirp->di;
		}
	}
	return NULL;
}

int fs_closeddir(fdDir *dirp) {
	if (dirp == NULL){
		printf("dirp is null\n");
		return -1;
	}
	free(dirp->di);
	dirp->di = NULL;
	free(dirp);
	dirp = NULL;

	return 0;
}



int fs_stat(const char *path, struct fs_stat *buf) {







	return 0;
}
