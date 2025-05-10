#include "../include/acnn.h"
#include <string.h>

uint32_t allocate_data_block(uint8_t *image, uint32_t total_blocks, struct superblock *sb) {
    if (!image || !sb) {
        log_error("Invalid arguments passed to allocate_data_block");
        return (uint32_t)-1;
    }

    uint8_t *block_bitmap = image + BLOCK_SIZE * BLOCK_BITMAP_BLOCK;

    for (uint32_t i = 0; i < total_blocks; i++) {
        uint32_t byte_idx = i / 8;
        uint32_t bit_idx = i % 8;

        if (!(block_bitmap[byte_idx] & (1 << bit_idx))) {
            block_bitmap[byte_idx] |= (1 << bit_idx);
            sb->free_blocks--;

            memset(image + i * BLOCK_SIZE, 0, BLOCK_SIZE);

            return i;
        }
    }

    log_error("No free blocks available in block bitmap");
    return (uint32_t)-1;
}

void free_data_block(uint8_t *image, struct superblock *sb, uint32_t block_index) {
    if (!image || !sb) {
        log_error("Invalid arguments passed to free_data_block");
        return;
    }

    if (block_index >= sb->total_blocks) {
        log_error("Block index out of bounds: %u", block_index);
        return;
    }

    uint8_t *block_bitmap = image + BLOCK_SIZE * BLOCK_BITMAP_BLOCK;
    uint32_t byte_idx = block_index / 8;
    uint32_t bit_idx = block_index % 8;

    block_bitmap[byte_idx] &= ~(1 << bit_idx);
    sb->free_blocks++;

    memcpy(image + BLOCK_SIZE * SUPERBLOCK_BLOCK, sb, sizeof(struct superblock));
}

void initialize_reserved_blocks(uint8_t *image, struct superblock *sb) {
    uint8_t *block_bitmap = image + BLOCK_SIZE * BLOCK_BITMAP_BLOCK;
    for (uint32_t i = 0; i <= INODE_TABLE_BLOCK; i++) {
        uint32_t byte_idx = i / 8;
        uint32_t bit_idx = i % 8;
        block_bitmap[byte_idx] |= (1 << bit_idx);
        sb->free_blocks--;
    }
}