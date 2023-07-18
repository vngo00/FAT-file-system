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
    printf("[ FS GETCWD ] : Attempting to get current working directory...\n");

    if (cwd == NULL || cwd[0] == '\0') {
        printf("[ FS GETCWD ] : Error, cwd is NULL or empty.\n");
        return NULL;
    }

    printf("[ FS GETCWD ] : Current working directory path: %s\n", cwd);

    if (strlen(cwd) >= 255) {
        printf("[ FS GETCWD ] : Error, path buffer is too small. Need size: %ld\n", strlen(cwd) + 1);
        return NULL;
    }

    printf("[ FS GETCWD ] : Current working directory: %s\n", cwd);

    return cwd;
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
Directory_Entry* get_target_directory(Directory_Entry* current_dir_ent, char* token) {
    printf("[ GET TARGET DIRECTORY ] : Getting target directory, token: %s\n", token);

    Directory_Entry *entries = malloc(DIRECTORY_MAX_LENGTH * 512);
    if (entries == NULL) {
        printf("[ GET TARGET DIRECTORY ] : Failed to allocate memory for entries.\n");
        return NULL;
    }
    printf("[ GET TARGET DIRECTORY ] : Allocated memory for directory entries\n");

    int readResult = LBAread(entries, 1, current_dir_ent->entries_array_location);
    if (readResult < 0) {
        printf("[ GET TARGET DIRECTORY ] : LBAread failed.\n");
        free(entries);
        return NULL;
    }
    printf("[ GET TARGET DIRECTORY ] : Read entries from disk at location: %d\n", current_dir_ent->entries_array_location);

    Directory_Entry *retVal = NULL;

    for (int i = 0; i < 64; i++) {
        printf("[ GET TARGET DIRECTORY ] : Checking entry number: %d, name: %s, attribute: %d\n", i, entries[i].dir_name, entries[i].dir_attr);

        if (strcmp(entries[i].dir_name, token) == 0) {
            printf("[ GET TARGET DIRECTORY ] : Found matching directory entry at index: %d\n", i);

            retVal = malloc(sizeof (Directory_Entry));
            if (!retVal) {
                printf("[ GET TARGET DIRECTORY ] : Failed to allocate memory for retVal.\n");
                free(entries);
                return NULL;
            }
            printf("[ GET TARGET DIRECTORY ] : Allocated memory for return value\n");

            *retVal = entries[i]; 

            printf("[ GET TARGET DIRECTORY ] : Copied found entry to return value\n");
            printf("[ FS SETCWD ] : Directory name to be set: '%s'\n", retVal->dir_name);
            fs_setcwd(retVal->dir_name);

            break;
        }
    }

    if (!retVal) {
        printf("[ GET TARGET DIRECTORY ] : No matching directory entry found. Returning NULL.\n");
    }

    free(entries);
    return retVal;
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
        retVal = get_target_directory(current_dir_ent, token);
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

int fs_isFile(char *filename) {
    Directory_Entry* directory_entry = parse_directory_path(filename);
    if (directory_entry == NULL) {
        printf("[ FS ISFILE ] : File '%s' not found.\n", filename);
        return 0;
    }

    int is_file = !check_directory_attribute(directory_entry->dir_attr);
    if (is_file) {
        printf("[ FS ISFILE ] : File '%s' is a regular file.\n", filename);
    } else {
        printf("[ FS ISFILE ] : File '%s' is not a regular file.\n", filename);
    }

    free(directory_entry);
    return is_file;
}

int fs_isDir(char *pathname) {
    Directory_Entry* directory_entry = parse_directory_path(pathname);
    if (directory_entry == NULL) {
        printf("[ FS ISDIR ] : Directory '%s' not found.\n", pathname);
        return 0;
    }

    int is_dir = check_directory_attribute(directory_entry->dir_attr);
    if (is_dir) {
        printf("[ FS ISDIR ] : Directory '%s' is a directory.\n", pathname);
    } else {
        printf("[ FS ISDIR ] : Directory '%s' is not a directory.\n", pathname);
    }

    free(directory_entry);
    return is_dir;
}


int fs_delete(char* filename) {
	// if not file do no delet
	if (fs_isDir(filename) == 1){
		printf("Can't delete a directory\n");
	}
	
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
	// for now i will jsut point it to null
	entry.parent[entry.index] = NULL;

	return 0;	
}

