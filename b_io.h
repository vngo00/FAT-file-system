/**************************************************************
* Class:  CSC-415-01 Summer 2023
* Names: Tyler Fulinara, Rafael Sant Ana Leitao, Anthony Silva , Vinh Ngo Rafael Fabiani
* Student IDs: 922002234, 920984945,
922907645, 921919541,
922965105
* GitHub Name: rf922
* Group Name: MKFS
* Project: Basic File System
* File: b_io.h
*
* Description: Interface of basic I/O functions
*
**************************************************************/

#ifndef _B_IO_H
#define _B_IO_H
#include <fcntl.h>

typedef int b_io_fd;

// Opens a file with the given filename and flags. 
// Returns a file descriptor.
b_io_fd b_open (char * filename, int flags);

// Reads count bytes from the file described by fd into the buffer. 
// Returns the number of bytes read.
int b_read (b_io_fd fd, char * buffer, int count);

// Writes count bytes from the buffer into the file described by fd.
// Returns the number of bytes written.
int b_write (b_io_fd fd, char * buffer, int count);

// It moves the file offset of the file description indicated by fd 
// by offset bytes according to whence.
// Returns the resulting offset location in bytes from the beginning of the file.
int b_seek (b_io_fd fd, off_t offset, int whence);

// Closes the file descriptor fd.
// Returns zero on success, or -1 if error.
int b_close (b_io_fd fd);

#endif

