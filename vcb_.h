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
* Description: VCB header file containing volume control block 
* structure. Also contains signatures of functions for
* handling the VCB.
**************************************************************/
#ifndef _VCB__H
#define _VCB__H
#include <stdint.h>

typedef struct VCB {
    uint32_t total_blocks_32;     // 4 bytes
    uint32_t FAT_start;           // Start of the FAT, 4 bytes
    uint32_t FAT_size_32;         // 4 bytes
    uint32_t root_cluster;        // rootDir, 4 bytes
    uint32_t free_space;          // 4 bytes
    uint32_t magic_number;        // Magic Number, 4 bytes
    uint32_t entries_per_dir;     // 4 bytes
    uint16_t bytes_per_block;     // blockSize, 2 bytes
    uint16_t reserved_blocks_count;   // 2 bytes
    uint16_t root_entry_count;    // 2 bytes
} VCB;

VCB * vcb;

#define     VCB_BLOCK_LOCATION              0
#define 	MAGIC_NUMBER     9090

int init_volume_control_block(uint32_t number_of_blocks, uint16_t block_size);
int read_vcb_from_disk(VCB *volume_control_block);
int is_init(VCB * volumeControlBlock);

#endif // _VCB__H
