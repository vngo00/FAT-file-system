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