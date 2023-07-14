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
* File: vcb_.h
*
* Description: VCB handling header file.
**************************************************************/
#ifndef _VCB__H
#define _VCB__H
#include <stdint.h> // This includes standard integer types like uint32_t and uint16_t

typedef struct VCB {//size of fields, desc of field
    uint32_t total_blocks_32;     // 4 bytes, the total number of blocks in the file system
    uint32_t FAT_start;           // 4 bytes, Start of the FAT Table 
    uint32_t FAT_size_32;  // 4 bytes, total blocks in the FAT
    uint32_t root_cluster;        // 4 bytes, location of the root
    uint32_t free_space;          // 4 bytes, amount of free blocks
    uint32_t magic_number;        // 4 bytes, Magic Number
    uint32_t entries_per_dir;     // 4 bytes
    uint16_t bytes_per_block;     //  2 bytes, blockSize,
    uint16_t reserved_blocks_count;   // 2 bytes, reserved blocks count
    uint16_t root_entry_count;    // 2 bytes, entries in the root
} VCB;

extern VCB * vcb;

#define     VCB_BLOCK_LOCATION              0
#define 	MAGIC_NUMBER     9090

// The `vcb_init` funtion initializes the volume control block (VCB). 
// It either reads the VCB from disk or initizializes it with defualt values if not alreaddy initiaized. 
// If there are issues during any step of this process, it returns -1 after freeing any allocated memory.
int vcb_init(uint32_t number_of_blocks, uint16_t block_size);

// The `vcb_read_from_disk` function reads the VCB from disk using the LBAread method. 
// If the read opration is unsuccesfull, it returns -1.
int vcb_read_from_disk(VCB *vcb);

// The `vcb_is_init` function checks if the VCB is already initialized by checking its magic number. 
// It returns 0 if the VCB is not initialized or if the VCB pointer is null.
int vcb_is_init(VCB *volume_control_block);


#endif // _VCB__H
