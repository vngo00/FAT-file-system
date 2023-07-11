/**************************************************************
* File: vcb_.c
**************************************************************/
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#include "fsLow.h"
#include "mfs.h"
#include "vcb_.h"
#include "FAT32.h"
#include "root_init.h"


VCB* vcb = NULL;


// Initializes the VCB, FAT, and Root Directory
int vcb_init(uint32_t number_of_blocks, uint16_t block_size){

 vcb = (VCB*)malloc(block_size);
    
    int ret_val = vcb_read_from_disk(vcb); 

    if(ret_val == -1){
        printf("Something went wrong when reading vcb from the SampleVolume.");
        return ret_val;
    }

    if (!vcb_is_init(vcb)) {
        vcb->total_blocks_32 = number_of_blocks; // total_blocks_32
        vcb->FAT_size_32 = blocks_per_fat; // FAT_size_32
        vcb->root_cluster = init_directory(block_size, NULL, "/"); // root_cluster
        vcb->free_space = fat_init(number_of_blocks, block_size); // free_space
        vcb->magic_number = MAGIC_NUMBER; // magic_number
        vcb->entries_per_dir = 4096; // entries_per_dir -- to be filled
        vcb->bytes_per_block = 512; // bytes_per_block
        vcb->reserved_blocks_count = 154; // reserved_blocks_count
        vcb->root_entry_count = 0; // root_entry_count -- to be filled


        LBAwrite(vcb, 1,  VCB_BLOCK_LOCATION );
    } else {

    
    return 0;
}

int vcb_read_from_disk(VCB *vcb){
    // The return value of LBARead
    int ret_value = 0;
    ret_value = LBAread(vcb, 1, VCB_BLOCK_LOCATION);

    // Return -1 for failure to read block
    if(ret_value != 1){
        return -1;
    }
    return 0;
}


int vcb_is_init(VCB *volume_control_block){
    return (vcb->magic_number == MAGIC_NUMBER);
}
