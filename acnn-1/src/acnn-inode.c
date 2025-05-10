#include "../include/acnn.h"
#include <string.h>

uint32_t allocate_inode(uint8_t *image, struct superblock *sb) {
    if (!image || !sb) {
        log_error("Invalid arguments passed to allocate_inode");
        return (uint32_t)-1;
    }

    uint8_t *inode_bitmap = image + BLOCK_SIZE * INODE_BITMAP_BLOCK;

    for (uint32_t i = 0; i < sb->total_inodes; i++) {
        uint32_t byte_idx = i / 8;
        uint32_t bit_idx = i % 8;

        if (!(inode_bitmap[byte_idx] & (1 << bit_idx))) {
            inode_bitmap[byte_idx] |= (1 << bit_idx);
            sb->free_inodes--;

            return i;
        }
    }

    log_error("No free inodes available in inode bitmap");
    return (uint32_t)-1;
}