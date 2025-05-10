#include "../include/acnn.h"
#include <string.h>

int create_directory(uint8_t *image, struct superblock *sb, uint32_t parent_inode_idx, const char *name) {
    if (!image || !sb || !name) {
        log_error("Invalid arguments passed to create_directory");
        return ERR_INVALID_ARGUMENTS;
    }

    uint32_t dir_inode_idx = allocate_inode(image, sb);
    if (dir_inode_idx == (uint32_t)-1) {
        log_error("No free inodes available for directory '%s'", name);
        return ERR_NO_FREE_INODES;
    }

    log_info("Allocated inode %u for directory '%s'", dir_inode_idx, name);

    uint32_t dir_data_block = allocate_data_block(image, sb->total_blocks, sb);
    if (dir_data_block == (uint32_t)-1) {
        log_error("No free blocks available for directory '%s'", name);
        return ERR_NO_FREE_BLOCKS;
    }

    log_info("Allocated data block %u for directory '%s'", dir_data_block, name);

    struct inode *dir_inode = (struct inode *)(image + BLOCK_SIZE * INODE_TABLE_BLOCK) + dir_inode_idx;
    memset(dir_inode, 0, sizeof(struct inode));
    dir_inode->direct_blocks[0] = dir_data_block;
    dir_inode->size = 0;

    struct dir_entry *entries = (struct dir_entry *)(image + BLOCK_SIZE * dir_data_block);
    memset(entries, 0, BLOCK_SIZE);

    log_info("Initialized directory block %u for '%s'", dir_data_block, name);

    if (add_dir_entry(image, sb, parent_inode_idx, dir_inode_idx, name) != 0) {
        log_error("Failed to add directory entry for '%s'", name);
        return -1;
    }

    log_info("Directory '%s' created successfully with inode index %u", name, dir_inode_idx);
    return 0; 
}

int delete_directory(uint8_t *image, struct superblock *sb, uint32_t dir_inode_idx) {
    if (!image || !sb) {
        log_error("Invalid arguments passed to delete_directory");
        return ERR_INVALID_ARGUMENTS;
    }

    if (dir_inode_idx >= sb->total_inodes) {
        log_error("Invalid directory inode index: %u", dir_inode_idx);
        return ERR_INVALID_INODE_INDEX;
    }

    struct inode *dir_inode = (struct inode *)(image + BLOCK_SIZE * INODE_TABLE_BLOCK) + dir_inode_idx;

    for (int i = 0; i < 10; i++) {
        uint32_t block = dir_inode->direct_blocks[i];
        if (block != 0) {
            log_info("Freeing data block %u for directory inode %u", block, dir_inode_idx);
            free_data_block(image, sb, block);
            dir_inode->direct_blocks[i] = 0;
        }
    }

    memset(dir_inode, 0, sizeof(struct inode));

    uint8_t *inode_bitmap = image + BLOCK_SIZE * INODE_BITMAP_BLOCK;
    inode_bitmap[dir_inode_idx / 8] &= ~(1 << (dir_inode_idx % 8));
    sb->free_inodes++;

    struct inode *parent_inode = (struct inode *)(image + BLOCK_SIZE * INODE_TABLE_BLOCK) + sb->root_inode;

    for (int i = 0; i < 10; i++) {
        uint32_t block = parent_inode->direct_blocks[i];
        if (block == 0) continue;

        struct dir_entry *entries = (struct dir_entry *)(image + block * BLOCK_SIZE);
        int entries_in_block = BLOCK_SIZE / sizeof(struct dir_entry);

        for (int j = 0; j < entries_in_block; j++) {
            if (entries[j].inode == dir_inode_idx) {
                log_info("Removing directory entry for inode %u", dir_inode_idx);
                entries[j].inode = 0; // Clear the directory entry
                memset(entries[j].name, 0, MAX_FILENAME_LEN); // Clear the name
                parent_inode->size -= sizeof(struct dir_entry);
                return 0; // Success
            }
        }
    }

    log_error("Directory entry for inode %u not found in parent directory", dir_inode_idx);
    return ERR_INVALID_ARGUMENTS;
}

void list_directory(uint8_t *image, struct superblock *sb, uint32_t dir_inode_idx) {
    if (!image || !sb) {
        log_error("Invalid arguments passed to list_directory");
        return;
    }

    struct inode *dir_inode = (struct inode *)(image + BLOCK_SIZE * INODE_TABLE_BLOCK) + dir_inode_idx;

    log_info("Listing directory contents (inode %u):", dir_inode_idx);
    for (int i = 0; i < 10; i++) {
        uint32_t block = dir_inode->direct_blocks[i];
        if (block == 0) continue;

        struct dir_entry *entries = (struct dir_entry *)(image + BLOCK_SIZE * block);
        int entries_in_block = BLOCK_SIZE / sizeof(struct dir_entry);

        for (int j = 0; j < entries_in_block; j++) {
            if (entries[j].inode != 0) {
                log_info("  %s (inode %u)", entries[j].name, entries[j].inode);
            }
        }
    }
}

int add_dir_entry(uint8_t *image, struct superblock *sb, uint32_t dir_inode_idx, uint32_t file_inode_idx, const char *name) {
    if (dir_inode_idx >= sb->total_inodes) {
        log_error("Invalid directory inode index %u while adding entry '%s'", dir_inode_idx, name);
        return -1;
    }

    struct inode *dir_inode = (struct inode *)(image + BLOCK_SIZE * INODE_TABLE_BLOCK) + dir_inode_idx;

    struct dir_entry entry = {
        .inode = file_inode_idx,
    };
    strncpy(entry.name, name, MAX_FILENAME_LEN - 1);
    entry.name[MAX_FILENAME_LEN - 1] = '\0'; 

    for (int i = 0; i < 10; i++) {
        uint32_t block = dir_inode->direct_blocks[i];

        if (block == 0) {
            block = allocate_data_block(image, sb->total_blocks, sb);
            if (block == (uint32_t)-1) {
                log_error("Failed to allocate block for directory '%s'", name);
                return -1;
            }
            dir_inode->direct_blocks[i] = block;

            struct dir_entry *entries = (struct dir_entry *)(image + block * BLOCK_SIZE);
            memset(entries, 0, BLOCK_SIZE);
        }

        struct dir_entry *entries = (struct dir_entry *)(image + block * BLOCK_SIZE);
        int entries_in_block = BLOCK_SIZE / sizeof(struct dir_entry);

        for (int j = 0; j < entries_in_block; j++) {
            if (entries[j].inode == 0) { 
                log_info("Adding directory entry: %s (inode %u) at block %u, entry %d", name, file_inode_idx, block, j);
                entries[j] = entry;
                dir_inode->size += sizeof(struct dir_entry);
                return 0;
            }
        }
    }

    log_error("No space available to add directory entry '%s'", name);
    return -1; 
}

int find_dir_entry(uint8_t *image, struct superblock *sb, uint32_t dir_inode_idx, const char *name) {
    if (dir_inode_idx >= sb->total_inodes) {
        log_error("Invalid inode index: %u", dir_inode_idx);
        return -1;
    }

    struct inode *dir_inode = (struct inode *)(image + BLOCK_SIZE * INODE_TABLE_BLOCK) + dir_inode_idx;

    for (int i = 0; i < 10; i++) {
        uint32_t block = dir_inode->direct_blocks[i];
        if (block == 0) continue;

        struct dir_entry *entries = (struct dir_entry *)(image + block * BLOCK_SIZE);
        int entries_in_block = BLOCK_SIZE / sizeof(struct dir_entry);

        for (int j = 0; j < entries_in_block; j++) {
            log_info("Checking directory entry: '%s' (inode %u)", entries[j].name, entries[j].inode);
            if (entries[j].inode != 0 && strncmp(entries[j].name, name, MAX_FILENAME_LEN) == 0) {
                log_info("Found directory entry: '%s' (inode %u)", name, entries[j].inode);
                return entries[j].inode;
            }
        }
    }

    log_error("Directory entry '%s' not found", name);
    return -1; 
}