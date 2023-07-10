#ifndef __FAT_H__
#define __FAT_H__

#define FAT_BLOCK_START 1

int init_FAT(uint64_t numblocks, uint61_t block_size);
int read_FAT_from_disk();
uint32_t alloate_blocks(int blbocks_to_allocate);
uint32_t release_blocks(int block);
void allocate_additional_blocks(uint32_t first_block, int blocks_to_allocate);
uint32_t get_next_block(int current_block);


#endif //__FAT_H__


