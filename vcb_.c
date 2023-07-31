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
* File: vcb_.c
*
* Description: VCB handling source file.
**************************************************************/
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "fsLow.h"
#include "mfs.h"
#include "vcb_.h"
#include "FAT.h"
#include "root_init.h"


VCB* vcb = NULL; //before setup, set to NULL
int entries_per_dir = 128; //number of entries per each directory

/**
 * This function is used for initalizing the volume control block
 *
 * @param number_of_blocks - A uint32_t representing the number of blocks needed for the VCB
 * @param block_size - A uint32_t representing the block size for each of the blocks needed for the VCB
 *
 * @return - On success of initalizng the volume control block return 0
 *         - On failure to allocate memory, return -1
 *         - On failure to read VCB from disk, return -1
 *         - On failure to write VCB to disk, return -1
 *         
 */
int vcb_init(uint32_t number_of_blocks, uint16_t block_size) {
    printf("[ VCB INIT ] : Initializing Volume Control Block...\n");

    vcb = (VCB*) malloc(block_size);
    if (!vcb) {
        printf("[ VCB INIT ] : Failed to allocate memory for VCB.\n");
        return -1;
    }

    int ret_val = vcb_read_from_disk(vcb);

    if (ret_val == -1) {
        printf("[ VCB INIT ] : Failed to read VCB from disk.\n");
        free(vcb);
        return ret_val;
    }

    vcb->total_blocks_32 = 19531;
    vcb->FAT_size_32 = 153;
    vcb->root_cluster = 154;
    vcb->reserved_blocks_count = 154;

    vcb->free_space = 19531 - 155;
    vcb->magic_number = MAGIC_NUMBER;
    vcb->entries_per_dir = 128;
    vcb->bytes_per_block = block_size;


    printf("[ VCB INIT ] : Writing VCB to disk, root cluster: %d\n", vcb->root_cluster);
    if (LBAwrite(vcb, 1, VCB_BLOCK_LOCATION) != 1) {
        printf("[ VCB INIT ] : Failed to write VCB to disk.\n");
        free(vcb);
        return -1;
    }
    return 0;
}

/**
 * This helper function is used for reading vcb from disk
 *
 * @param vcb - A vcb representing which vcb you want to read
 *
 * @return - On success of reading the volume control block from disk return 0
 *         - On failure to read VCB from disk, return -1
 *         
 */
int vcb_read_from_disk(VCB *vcb) {
    printf("[ VCB READ FROM DISK ] : Reading VCB from disk...\n");

    int ret_value = LBAread(vcb, 1, VCB_BLOCK_LOCATION);
    if (ret_value != 1) {
        printf("[ VCB READ FROM DISK ] : Failed to read VCB from disk.\n");
        return -1;
    }
    printf("[ VCB READ FROM DISK ] : Successfully read VCB from disk.\n");
    return 0;
}

/**
 * This helper function is used to check if the vcb is initalized
 *
 * @return - If vcb is initalized return 0
 *         - If Volume control block is not initalized return -1
 *         
 */
int vcb_is_init() {
    printf("[ VCB IS INIT ] : Checking if VCB is initialized...\n");
    
    vcb = malloc(512);
    vcb_read_from_disk(vcb);
    if (vcb == NULL) {
        printf("[ VCB IS INIT ] : Volume Control Block is not initialized.\n");
        return 0;
    }

    if (vcb->magic_number == MAGIC_NUMBER) {
        printf("[ VCB IS INIT ] : Volume Control Block is initialized.\n");
        return 1;
    } else {
        printf("[ VCB IS INIT ] : Volume Control Block is not initialized.\n");
        return 0;
    }
}
