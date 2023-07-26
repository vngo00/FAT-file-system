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
#include "FAT.h"
// Maximum number of file descriptors that can be open at the same time in the system.
#define MAXFCBS 20

// Size of the chunks that the system will use to read from or write to the files.
#define B_CHUNK_SIZE 512

// Global variable.
extern int bytes_per_block;

int get_last_block(int location);

typedef struct file_info {
	char file_name[NAME_MAX_LENGTH];	// file name
	int file_size;						// file size in bytes
	int location;						// starting logical block in disk
	int blocks;							// total blocks of file in disk
} file_info;


/**
 * The function retrieves the information of a file given its name.
 *
 * @param fname - A pointer to a string containing the name of the file.
 *                This string is expected to include the file's complete path in the file system.
 *                
 * @return - On success, this function returns a pointer to a `file_info` struct.
 * 		   - If fname is a directory, it returns NULL.
 */
file_info * get_file_info(char *fname) {
	// Check if the given path is a directory.
	if (fs_isDir(fname) == 1) {
		// If path is a directory, exits get_file_info returning NULL.
		return NULL;
	}

	// Declare an instance of parsed_entry struct to hold the parsed 
	// information of the directory path.
	parsed_entry entry;

	// Statement to indicate the directory path does not exist.
	// If parse function returns -1, exits get_file_info returning NULL.
	if ( parse_directory_path(fname, &entry) == -1) {
		printf(" get fileinffo %s does not exists", entry.name);
		return NULL;
	}

	// Checks if the parent directory of file exists.
	// If parent directory does not exist, exits get_file_info returning NULL.
	if (entry.parent == NULL) {
		printf(" get fileinfo invalid file\n");
		return NULL;
	}

	// Allocate memory on the heap to create a new file_info object.
	file_info *finfo = malloc(sizeof(file_info));

	// Checks if malloc was successful in allocating memory.
	// exits get_file_info returning NULL.
	if (finfo == NULL) {
    	printf("Memory allocation failed\n");
    	return NULL;
	}

	// If entry has a valid index, file or directory exists.
	if (entry.index != -1) {

		// Copy the file or directory name from the entry to the finfo.
		strcpy(finfo->file_name, entry.parent[entry.index].dir_name);

		// Set the file size in finfo to the size in entry.
		finfo->file_size = entry.parent[entry.index].dir_file_size;

		// Set the location of the file or directory in finfo to the first cluster number in entry.
		finfo->location = entry.parent[entry.index].dir_first_cluster;

		// Number of blocks occupied by the file or directory.
		int blocks = (finfo->file_size + bytes_per_block -1) / bytes_per_block;

		// Set the number of blocks in finfo.
		finfo->blocks = blocks;
	} else {

		// If entry index is -1, it's not a valid file or directory,
		// so it sets the name in finfo to an empty string.
		strcpy(finfo->file_name, "");
	}

	// If the parent of the parsed entry is not the root directory or the current directory,
	// free the memory allocated for the parent.
	if (entry.parent != root_directory && entry.parent != current_directory) {
		free(entry.parent);
		// After freeing memory set the pointer to NULL to avoid dangling pointers.
		entry.parent = NULL;
	}
	// Returns file_info structure that contains information about the file or directory.
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
		fcbArray[returnFd].blocks_read = fcbArray[returnFd].fi->blocks;
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

    // Determine new file position according to the directive `whence`
    switch (whence)
    {
        case SEEK_SET:
			// Seek from the beginning of the file
			if (offset >= 0 && offset <= fcbArray[fd].fi->file_size) {
				fcbArray[fd].file_size_index = offset;
			} else if (offset < 0 && -offset <= fcbArray[fd].fi->file_size) {
				// if offset is negative, it moves backwards from end
				fcbArray[fd].file_size_index = fcbArray[fd].fi->file_size + offset; 
			} else {
				return (-1); // Invalid offset
			}
			break;

        case SEEK_CUR:
			// Calculate the new offset
			off_t new_offset = fcbArray[fd].file_size_index + offset;

			// Check if the new offset is valid
			if ((new_offset >= 0) && (new_offset <= fcbArray[fd].fi->file_size)) {
				fcbArray[fd].file_size_index = new_offset;
			} else {
				return (-1); // Invalid offset
			}
			break;

        case SEEK_END:
			// Seek from the end of the file
			if (offset <= 0 && -offset <= fcbArray[fd].fi->file_size) {
				fcbArray[fd].file_size_index = fcbArray[fd].fi->file_size + offset; // offset is expected to be negative or zero here
			} else if (offset > 0 && offset <= fcbArray[fd].fi->file_size) {
				fcbArray[fd].file_size_index = offset; // if offset is positive, it moves forward from beginning
			} else {
				return (-1); // Invalid offset
			}
			break;

        default:
            return (-1); // Invalid whence
    }

    // Calculate block and index within the block for the new position
    int block_num = fcbArray[fd].file_size_index / bytes_per_block;
    fcbArray[fd].index = fcbArray[fd].file_size_index % bytes_per_block;
    
    // Traverse through FAT table to get to the correct block
    int current_location = fcbArray[fd].fi->location;
    for (int i = 0; i < block_num; i++) {
        current_location = get_next_block(current_location);
    }
    fcbArray[fd].current_location = current_location;

    // Refresh buffer with the new block data
    fcbArray[fd].buflen = read_block(fcbArray[fd].buf, fcbArray[fd].current_location);

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


int get_last_block(int location) {
	int prev = -1;
	int curr = location;
	while (curr != -1) {
		prev = curr;
		curr = get_next_block(curr);
	}
	return prev;
}

int main() {

    b_init(); // Initialize the file system
    
    // Try opening a file
    b_io_fd fd = b_open("test.txt", O_RDWR);
    if (fd < 0) {
        printf("Error opening the file\n");
        return -1;
    }

    // Test SEEK_SET
    int offset = b_seek(fd, 100, SEEK_SET);
    if (offset != 100) {
        printf("Error seeking to offset 100 from beginning of file. Returned offset: %d\n", offset);
    } else {
        printf("Successfully sought to offset 100 from beginning of file.\n");
    }

    // Test SEEK_CUR
    offset = b_seek(fd, 50, SEEK_CUR);
    if (offset != 150) {
        printf("Error seeking to offset 50 from current position. Returned offset: %d\n", offset);
    } else {
        printf("Successfully sought to offset 50 from current position.\n");
    }

    // Test SEEK_END
    offset = b_seek(fd, -50, SEEK_END);
    if (offset != fcbArray[fd].fi->file_size - 50) { // You should replace this with the correct file size
        printf("Error seeking to offset 50 from end of file. Returned offset: %d\n", offset);
    } else {
        printf("Successfully sought to offset 50 from end of file.\n");
    }

    // Close the file
    b_close(fd);

    return 0;
}
