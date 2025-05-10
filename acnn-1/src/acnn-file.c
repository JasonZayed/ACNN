#include "../include/acnn.h"
#include <string.h>

int create_file(uint8_t *image, struct superblock *sb, uint32_t inode_idx, const char *data) {
    if (!image || !sb || !data) {
        log_error("Invalid arguments passed to create_file");
        return ERR_INVALID_INODE_INDEX;
    }

    if (inode_idx >= sb->total_inodes) {
        log_error("Inode index out of bounds: %u", inode_idx);
        return ERR_INVALID_INODE_INDEX;
    }

    struct inode *file_inode = (struct inode *)(image + BLOCK_SIZE * INODE_TABLE_BLOCK) + inode_idx;

    uint32_t data_block_index = 0;
    uint32_t data_offset = 0;
    uint32_t remaining_data = strlen(data);

    while (remaining_data > 0) {
        uint32_t data_block = allocate_data_block(image, sb->total_blocks, sb);
        if (data_block == (uint32_t)-1) {
            log_error("No free data blocks available for inode %u", inode_idx);
            return ERR_NO_FREE_BLOCKS;
        }

        uint32_t chunk_size = (remaining_data > BLOCK_SIZE) ? BLOCK_SIZE : remaining_data;
        memcpy(image + BLOCK_SIZE * data_block, data + data_offset, chunk_size);

        if (data_block_index < 10) {
            file_inode->direct_blocks[data_block_index] = data_block;
        } else {
            log_error("File exceeds direct block limit for inode %u", inode_idx);
            return ERR_FILE_TOO_LARGE;
        }

        remaining_data -= chunk_size;
        data_offset += chunk_size;
        data_block_index++;
    }

    file_inode->size = strlen(data);

    uint8_t *inode_bitmap = image + BLOCK_SIZE * INODE_BITMAP_BLOCK;
    inode_bitmap[inode_idx / 8] |= (1 << (inode_idx % 8));
    sb->free_inodes--;

    memcpy(image + BLOCK_SIZE * SUPERBLOCK_BLOCK, sb, sizeof(struct superblock));

    log_info("File created successfully with inode index %u", inode_idx);
    return 0; 
}

int delete_file(uint8_t *image, struct superblock *sb, uint32_t dir_inode_idx, const char *filename) {
    if (!image || !sb || !filename) {
        log_error("Invalid arguments passed to delete_file");
        return ERR_INVALID_ARGUMENTS;
    }

    int file_inode_idx = find_dir_entry(image, sb, dir_inode_idx, filename);
    if (file_inode_idx == -1) {
        log_error("File '%s' not found in directory inode %u", filename, dir_inode_idx);
        return -1;
    }

    struct inode *file_inode = ((struct inode *)(image + BLOCK_SIZE * INODE_TABLE_BLOCK)) + file_inode_idx;

    for (int i = 0; i < 10; i++) {
        if (file_inode->direct_blocks[i] != 0) {
            free_data_block(image, sb, file_inode->direct_blocks[i]);
            file_inode->direct_blocks[i] = 0;
        }
    }

    memset(file_inode, 0, sizeof(struct inode));

    uint8_t *inode_bitmap = image + BLOCK_SIZE * INODE_BITMAP_BLOCK;
    inode_bitmap[file_inode_idx / 8] &= ~(1 << (file_inode_idx % 8));
    sb->free_inodes++;

    struct inode *dir_inode = ((struct inode *)(image + BLOCK_SIZE * INODE_TABLE_BLOCK)) + dir_inode_idx;
    for (int i = 0; i < 10; i++) {
        uint32_t block = dir_inode->direct_blocks[i];
        if (block == 0) continue;

        struct dir_entry *entries = (struct dir_entry *)(image + block * BLOCK_SIZE);
        int entries_in_block = BLOCK_SIZE / sizeof(struct dir_entry);

        for (int j = 0; j < entries_in_block; j++) {
            if (entries[j].inode == (uint32_t)file_inode_idx) { 
                entries[j].inode = 0; 
                memset(entries[j].name, 0, MAX_FILENAME_LEN); 
                dir_inode->size -= sizeof(struct dir_entry);
                log_info("Directory entry for file '%s' removed", filename);
                break;
            }
        }
    }

    memcpy(image + BLOCK_SIZE * SUPERBLOCK_BLOCK, sb, sizeof(struct superblock));

    log_info("File '%s' deleted successfully", filename);
    return 0; 
}

void read_file(uint8_t *image, struct inode *file_inode, char *buffer, size_t buffer_size) {
    if (!image || !file_inode || !buffer) {
        log_error("Invalid arguments passed to read_file");
        return;
    }

    if (buffer_size == 0) {
        log_error("Buffer size is zero");
        return;
    }

    size_t total_read = 0;
    for (int i = 0; i < 10 && total_read < file_inode->size; i++) {
        uint32_t block = file_inode->direct_blocks[i];
        if (block == 0) break;

        size_t to_read = BLOCK_SIZE;
        if (file_inode->size - total_read < BLOCK_SIZE)
            to_read = file_inode->size - total_read;

        memcpy(buffer + total_read, image + block * BLOCK_SIZE, to_read);
        total_read += to_read;
    }
    buffer[total_read] = '\0';
}

void write_file(uint8_t *image, struct superblock *sb, struct inode *file_inode, const char *data) {
    if (!image || !sb || !file_inode || !data) {
        log_error("Invalid arguments passed to write_file");
        return;
    }

    if (strlen(data) == 0) {
        log_error("Data to write is empty");
        return;
    }

    uint32_t data_offset = 0;
    uint32_t data_block_index = 0;
    uint32_t remaining = strlen(data);

    while (remaining > 0 && data_block_index < 10) {
        uint32_t block = allocate_data_block(image, sb->total_blocks, sb);
        if (block == (uint32_t)-1) {
            log_error("No more space available to write data");
            return;
        }

        size_t chunk = (remaining > BLOCK_SIZE) ? BLOCK_SIZE : remaining;
        memcpy(image + block * BLOCK_SIZE, data + data_offset, chunk);
        file_inode->direct_blocks[data_block_index++] = block;

        data_offset += chunk;
        remaining -= chunk;
        sb->free_blocks--;
    }
    file_inode->size = strlen(data);
}