/**************************************************************
* File: vcb_.c
**************************************************************/

// Importing necessary libraries and modules for this file.
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

// Including headers relevant to this file's operations.
#include "fsLow.h"
#include "mfs.h"
#include "vcb_.h"
#include "FAT.h"
#include "root_init.h"

// Declaration of the volume control block (VCB).
VCB* vcb = NULL;

// Function to initialize the volume control block (VCB).
// The VCB is first read from the disk. If it is not initialized, it is initialized with default values.
// If the initialization process fails at any step, it frees the allocated memory and returns -1.
int vcb_init(uint32_t number_of_blocks, uint16_t block_size) {
    // Allocate memory for the VCB
    vcb = (VCB*) malloc(block_size);
    if (!vcb) {
        printf("Failed to allocate memory for VCB.\n");
        return -1;
    }

    // Try to read the VCB from the disk
    int ret_val = vcb_read_from_disk(vcb);

    if (ret_val == -1) {
        printf("Failed to read VCB from disk.\n");
        free(vcb);
        return ret_val;
    }

    // If VCB is not initialized, initialize it
    if (!vcb_is_init(vcb)) {
        printf("VCB IS NOT INIT TRYING TO INIT IT NOW \n");
        vcb->total_blocks_32 = 19531;
        vcb->FAT_size_32 = 153;
        vcb->root_cluster = 154;
        vcb->free_space = fat_init(vcb->total_blocks_32, block_size);
        if (vcb->free_space < 0) {
            printf("Failed to initialize FAT.\n");
            free(vcb);
            return -1;
        }
        vcb->magic_number = MAGIC_NUMBER;
        vcb->entries_per_dir = 128;
        vcb->bytes_per_block = 512;
        vcb->reserved_blocks_count = 155;
        vcb->root_entry_count = 2;

        // Initialize the root directory
        if (init_directory(block_size, NULL, "/") == -1) {
            printf("Failed to initialize root directory.\n");
            free(vcb);
            return -1;
        }

        // Write the VCB to the disk
        printf("VALUE OF ROOT CLUSTER BEING WRITTEN : %d", vcb->root_cluster);
        if (LBAwrite(vcb, 1, VCB_BLOCK_LOCATION) != 1) {
            printf("Failed to write VCB to disk.\n");
            free(vcb);
            return -1;
        }
    } else {
        // If VCB is already initialized, read it from the disk and load the root directory
        printf("VCB IS INIT NOT GOING TO INIT IT NOW \n");
        ret_val = vcb_read_from_disk(vcb);
        if (ret_val == -1) {
            printf("Failed to read VCB from disk.\n");
            free(vcb);
            return ret_val;
        }
        ret_val = fat_read_from_disk();

        ret_val =  load_directory(block_size, NULL);
        if (ret_val == -1) {
            printf("Failed to load root directory.\n");
            free(vcb);
            return ret_val;
        }
    }

    // Allocate memory for the current working directory and set it to root
    Current_Working_Directory* current_wd = malloc(sizeof(Current_Working_Directory));
    if (!current_wd) {
        printf("Failed to allocate memory for current working directory.\n");
        free(vcb);
        return -1;
    }
    current_working_directory = current_wd;
    ret_val = fs_setcwd("/");
    if (ret_val != 0) {
        printf("Failed to set current working directory.\n");
        free(vcb);
        free(current_wd);
        return ret_val;
    }

    return 0;
}

// Function to read the VCB from disk. 
// It uses the LBAread method to read the VCB from the specific disk block.
// If the read operation fails, it logs an error message and returns -1.
int vcb_read_from_disk(VCB *vcb) {
    int ret_value = LBAread(vcb, 1, VCB_BLOCK_LOCATION);

    if (ret_value != 1) {
        printf("Failed to read VCB from disk.\n");
        return -1;
    }

    return 0;
}

// Function to check if the VCB is initialized by checking its magic number. 
// If the magic number is the expected value, the VCB is considered initialized.
// If the VCB pointer is null, it logs an error message and returns 0.
int vcb_is_init(VCB *volume_control_block) {
    if (volume_control_block == NULL) {
        printf("Volume Control Block is not initialized.\n");
        return 0;
    }
    printf("Volume Control Block is initialized.\n");

    return (vcb->magic_number == MAGIC_NUMBER);
}
