/**************************************************************
* Class:  CSC-415-01 Summer 2023
* Names: Tyler Fulinara, Rafael Sant Ana Leitao, Anthony Silva , Vinh Ngo Rafael Fabiani
* Student IDs: 922002234, 920984945,
922907645, 921919541,
922965105
* GitHub Name: rf922
* Group Name: MKFS
* Project: Basic File System
* File: b_io.c
*
* Description: Basic File System - Key File I/O Operations
*
**************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>			// for malloc
#include <string.h>			// for memcpy
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "b_io.h"
#include "mfs.h"

#define MAXFCBS 20
#define B_CHUNK_SIZE 512

extern int bytes_per_block;

typedef struct file_info {
	char file_name[NAME_MAX_LENGTH];	//file name
	int file_size;				// file size in bytes
	int location;				// starting logical block in disk
	int blocks;				// total blocks of file in disk
} file_info;

file_info * get_file_info(char *fname) {
	if (fs_isDir(fname) == 1) {
		return NULL;
	}
	parsed_entry entry;
	if ( parse_directory_path(fname, &entry) == -1) {
		printf(" get fileinffo %s does not exists", entry.name);
		return NULL;
	}

	if (entry.parent == NULL) {
		printf(" get fileinfo invalid file\n");
		return NULL;
	}
	file_info *finfo = malloc(sizeof(file_info));
	if (entry.index != -1) {
		strcpy(finfo->file_name, entry.parent[entry.index].dir_name);
		finfo->file_size = entry.parent[entry.index].dir_file_size;
		finfo->location = entry.parent[entry.index].dir_first_cluster;
		int blocks = (finfo->file_size + bytes_per_block -1) / bytes_per_block;
		finfo->blocks = blocks;
	} else {
		strcpy(finfo->file_name, "");
	}

	if (entry.parent != root_directory && entry.parent != current_directory) {
		free(entry.parent);
		entry.parent = NULL;
	}

	return finfo;
}



typedef struct b_fcb
	{
	/** TODO add al the information you need in the file control block **/
	file_info *fi;		// low level system file info
	char * buf;		//holds the open file buffer
	int index;		//holds the current position in the buffer
	int buflen;		//holds how many valid bytes are in the buffer
	int current_location;	// current block location 
	int blocks_read;	// blocks read so far
	int file_size_index;	// file offset
	int flags;		// mark the purpose when open the file
	} b_fcb;
	
b_fcb fcbArray[MAXFCBS];

int startup = 0;	//Indicates that this has not been initialized

//Method to initialize our file system
void b_init ()
	{
	//init fcbArray to all free
	for (int i = 0; i < MAXFCBS; i++)
		{
		fcbArray[i].buf = NULL; //indicates a free fcbArray
		}
		
	startup = 1;
	}

//Method to get a free FCB element
b_io_fd b_getFCB ()
	{
	for (int i = 0; i < MAXFCBS; i++)
		{
		if (fcbArray[i].buf == NULL)
			{
			return i;		//Not thread safe (But do not worry about it for this assignment)
			}
		}
	return (-1);  //all in use
	}
	
// Interface to open a buffered file
// Modification of interface for this assignment, flags match the Linux flags for open
// O_RDONLY, O_WRONLY, or O_RDWR
b_io_fd b_open (char * filename, int flags)
	{
	b_io_fd returnFd;

	//*** TODO ***:  Modify to save or set any information needed
	//
	//
	if (filename == NULL) return -1;	
	if ( (flags & O_RDONLY) ) {
		if ( (flags & O_TRUNC) || (flags & O_CREAT) || (flags & O_APPEND) || (flags & O_RDWR) ) {
			return -1;
		}
	}

	if ( (flags & O_WRONLY) && (flags & O_RDWR) ) return -1;
	if ( (flags & O_TRUNC) && (flags & O_APPEND) ) return 1;

	if (startup == 0) b_init();  //Initialize our system
	
	returnFd = b_getFCB();				// get our own file descriptor
	// check for error - all used FCB's
	if (returnFd == -1) return -1;

	fcbArray[returnFd].fi = (file_info *) get_file_info(filename);
	if (fcbArray[returnFd].fi == NULL) {
		printf("[OPEN] invalid filename\n");
		return -1;
	}

	if ( strcmp(fcbArray[returnFd].fi->file_name, "") == 0) {
		if ( !(flags & O_CREAT) ) { // don't want to make new if not exists
			printf("[OPEN] file does not exists\n");
			free(fcbArray[returnFd].fi);
			fcbArray[returnFd].fi = NULL;
			return -1;
		}
		if (fs_mkfile(filename) == -1) {
			free(fcbArray[returnFd].fi);
			fcbArray[returnFd].fi = NULL;
			return -1;
		}
		fcbArray[returnFd].fi = get_file_info(filename);
	} else if ( (flags & O_TRUNC) ) {
		fs_delete(filename);
		fs_mkfile(filename);
		fcbArray[returnFd].fi = get_file_info(filename);
	}

	
	fcbArray[returnFd].buf = (char *) malloc(B_CHUNK_SIZE);
	fcbArray[returnFd].index = 0;
	fcbArray[returnFd].buflen = 0;
	fcbArray[returnFd].current_location = fcbArray[returnFd].fi->location;
	fcbArray[returnFd].blocks_read = 0;
	fcbArray[returnFd].file_size_index = 0;
	fcbArray[returnFd].flags = flags;
	if ( (flags & O_APPEND) ) { // if append move the pointer to end of file
		fcbArray[returnFd].current_location = get_last_block(fcbArray[returnFd].fi->location);
		fcbArray[returnFd].blocks_read = fcbArray[returnFd].fi->bblocks;
		fcbArray[returnFd].file_size_index = fcbArray[returnFd].fi->file_size;
	}
	return (returnFd);						// all set
	}




// Interface to seek function	
int b_seek (b_io_fd fd, off_t offset, int whence)
{
    if (startup == 0) b_init();  // Initialize our system

    // Check that fd is between 0 and (MAXFCBS-1)
    if ((fd < 0) || (fd >= MAXFCBS))
    {
        return (-1); // Invalid file descriptor
    }

    // Check if offset is valid
    if (offset < 0)
    {
        return (-1); // Invalid offset
    }

    // Determine new file position according to the directive `whence`
    switch (whence)
    {
        case SEEK_SET:
            // Seek from the beginning of the file
            if (offset <= fcbArray[fd].fi->file_size) {
                fcbArray[fd].file_size_index = offset;
            } else {
                return (-1); // Invalid offset
            }
            break;

        case SEEK_CUR:
            // Seek from the current position in the file
            if ((fcbArray[fd].file_size_index + offset) <= fcbArray[fd].fi->file_size) {
                fcbArray[fd].file_size_index += offset;
            } else {
                return (-1); // Invalid offset
            }
            break;

        case SEEK_END:
            // Seek from the end of the file
            if (offset <= fcbArray[fd].fi->file_size) {
                fcbArray[fd].file_size_index = fcbArray[fd].fi->file_size - offset;
            } else {
                return (-1); // Invalid offset
            }
            break;

        default:
            return (-1); // Invalid whence
    }

    // After setting the file index, calculate the current location in blocks
    fcbArray[fd].current_location = fcbArray[fd].fi->location + (fcbArray[fd].file_size_index / bytes_per_block);

    // Calculate the index within the block
    fcbArray[fd].index = fcbArray[fd].file_size_index % bytes_per_block;

    // The buffer might be invalid now, so it resets buflen to 0.
    fcbArray[fd].buflen = 0;

	// Return the offset from the beginning
    return fcbArray[fd].file_size_index; 
}




// Interface to write function	
int b_write (b_io_fd fd, char * buffer, int count)
	{
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
		
		
	return (0); //Change this
	}



// Interface to read a buffer

// Filling the callers request is broken into three parts
// Part 1 is what can be filled from the current buffer, which may or may not be enough
// Part 2 is after using what was left in our buffer there is still 1 or more block
//        size chunks needed to fill the callers request.  This represents the number of
//        bytes in multiples of the blocksize.
// Part 3 is a value less than blocksize which is what remains to copy to the callers buffer
//        after fulfilling part 1 and part 2.  This would always be filled from a refill 
//        of our buffer.
//  +-------------+------------------------------------------------+--------+
//  |             |                                                |        |
//  | filled from |  filled direct in multiples of the block size  | filled |
//  | existing    |                                                | from   |
//  | buffer      |                                                |refilled|
//  |             |                                                | buffer |
//  |             |                                                |        |
//  | Part1       |  Part 2                                        | Part3  |
//  +-------------+------------------------------------------------+--------+


int b_read (b_io_fd fd, char * buffer, int count)
	{

	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
		
	return (0);	//Change this
	}
	
// Interface to Close the file	
int b_close (b_io_fd fd)
	{
	if ( (fd < 0) || (fd >= MAXFCBS)) {
		return -1;
	}
	free(fcbArray[fd].fi);
	fcbArray[fd].fi = NULL;
	free(fcbArray[fd].buf);
	fcbArray[fd].buf = NULL;
	fcbArray[fd].index = 0;
	fcbArray[fd].buflen = 0;
	fcbArray[fd].current_location = 0;
	fcbArray[fd].blocks_read = 0;
	fcbArray[fd].file_size_index = 0;
	fcbArray[fd]. flags = 0;

	return 0;
	}
