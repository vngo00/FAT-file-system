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
void build_absolute_path(char *buf, const char *pathname) {
    // If either the buffer or the pathname is not provided, we cannot proceed. Return early.
    if (!buf || !pathname) {
        printf("NULL argument.\n");
        return;
    }

     // Clear the buffer to remove any existing data.
    memset(buf, '\0', sizeof(buf));
    // If the pathname is not already an absolute path, append it to the current directory.
    if (pathname[0] != '/') {
        // Copy the current directory path to the buffer.
        strcat(buf, current_working_directory->path);

        // Append a slash if the current directory is not the root directory.
        if (strcmp(current_working_directory->path, "/") != 0) {
            strcat(buf, "/");
        }
        // Append the pathname to the current directory.
        strcat(buf, pathname);
    } else {
        // If the pathname is already an absolute path, copy it directly to the buffer.
        strcpy(buf, pathname);
    }
}
// This function changes the current working directory to the specified path.
// It allows us to navigate to a different directory within the file system.
int fs_setcwd(char *path) {
    // We need a valid path argument to proceed; otherwise, we cannot change the 
    // working directory.
    if (!path) {
        printf("NULL path.\n");
        return -1;
    }

    Directory_Entry *directory_entry;

    // If the requested path is the root directory ("/"), we reset the current working 
    // directory to the root.
    if (strcmp(path, "/") == 0) {
        clear_current_working_directory();

        // Set the path of the current working directory to the root ("/").
        strcpy(current_working_directory->path, "/");

        // Set the cluster of the current working directory to the root cluster obtained 
        // from the volume control block (VCB).
        current_working_directory->cluster = vcb->root_cluster;

        // Return 0 to indicate successful change to the root directory.
        return 0;
    }

    char absolute_path[255];
    // Create an absolute path based on the provided path
    build_absolute_path(absolute_path, path);

    // Try to find the directory specified by the absolute path
    directory_entry = parse_directory_path(absolute_path, 0);

    // If the directory cannot be found or it is not actually a directory, 
    // return -1 to indicate an error
    if (directory_entry == NULL || !check_directory_attribute(directory_entry->dir_attr)) {
        return -1;
    } 

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
Directory_Entry* get_target_directory(Directory_Entry* current_dir_ent, char* token, int pathIndex, int pathLength) {
    for (int i = 0; i < 4096; i++) {
        // It goes through each directory in the current one until it finds a match
        if (current_dir_ent[i].dir_name && strcmp(current_dir_ent[i].dir_name, token) == 0 && check_directory_attribute(current_dir_ent[i].dir_attr)) {
            Directory_Entry *retVal = malloc(sizeof(Directory_Entry));
            // Once we find the match, we copy it to our return value and then return it
            memcpy(retVal, &current_dir_ent[i], sizeof(Directory_Entry));
            return retVal;
        }
    }
    // If we can't find a match, we return NULL
    return NULL;
}