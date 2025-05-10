#define _XOPEN_SOURCE 700
#include "../include/acnn.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>

int main(int argc, char *argv[]) {
    char output_dir[PATH_MAX];
    const char *home = getenv("HOME");
    if (!home) {
        fprintf(stderr, "HOME environment variable is not set\n");
        return 1;
    }

    snprintf(output_dir, sizeof(output_dir), "%s/build-acnn/output", home);

    if (mkdir(output_dir, 0755) && errno != EEXIST) {
        perror("Failed to create output directory");
        return 1;
    }

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <disk size> (e.g. 4MB or 4194304)]\n", argv[0]);
        return 1;
    }

    size_t disk_size = parse_size(argv[1]);
    uint32_t total_blocks = disk_size / BLOCK_SIZE;

    uint8_t *image = calloc(1, disk_size);
    if (!image) {
        perror("Memory allocation failed");
        return 1;
    }

    FILE *out = NULL;

    struct superblock sb = {
        .magic_number = 0xA2C0F0F8,
        .block_size = BLOCK_SIZE,
        .total_blocks = total_blocks,
        .free_blocks = total_blocks - 10, 
        .total_inodes = total_blocks / 4,
        .free_inodes = (total_blocks / 4) - 1,
        .root_inode = 0,
        .inode_size = INODE_SIZE,
        .block_bitmap = BLOCK_BITMAP_BLOCK,
        .inode_bitmap = INODE_BITMAP_BLOCK,
    };

    memcpy(image + BLOCK_SIZE * SUPERBLOCK_BLOCK, &sb, sizeof(struct superblock));

    struct superblock *sb_check = (struct superblock *)(image + BLOCK_SIZE * SUPERBLOCK_BLOCK);
    log_info("Superblock verification:");
    log_info("Magic Number: 0x%X", sb_check->magic_number);
    log_info("Block Size: %u", sb_check->block_size);
    log_info("Total Blocks: %u", sb_check->total_blocks);
    log_info("Free Blocks: %u", sb_check->free_blocks);
    log_info("Total Inodes: %u", sb_check->total_inodes);
    log_info("Free Inodes: %u", sb_check->free_inodes);

    const char *test_data = "Hello, Block 32!";
    memcpy(image + BLOCK_SIZE * 32, test_data, strlen(test_data));

    log_info("Test data written to block 32: %s", test_data);

    uint32_t root_data_block = allocate_data_block(image, sb.total_blocks, &sb);
    if (root_data_block == (uint32_t)-1) {
        log_error("Failed to allocate root data block");
        cleanup(image, out);
        return 1;
    }

    initialize_reserved_blocks(image, &sb);

    struct dir_entry *root_entries = (struct dir_entry *)(image + BLOCK_SIZE * root_data_block);
    memset(root_entries, 0, BLOCK_SIZE); 

    struct inode *root_inode = (struct inode *)(image + BLOCK_SIZE * INODE_TABLE_BLOCK);
    root_inode->direct_blocks[0] = root_data_block;
    root_inode->size = 0; 

    char file_path[PATH_MAX];
    int n = snprintf(file_path, sizeof(file_path), "%s/acnn.img", output_dir);
    if(n < 0 || (unsigned)n >= sizeof(file_path)) {
        fprintf(stderr, "File path is too long or an encoding error occurred\n");
        cleanup(image, out);
        return 1;
    }
    out = fopen(file_path, "wb");
    if (!out) {
        perror("Failed to open output file");
        cleanup(image, out);
        return 1;
    }

    if (fwrite(image, 1, disk_size, out) != disk_size) {
        log_error("Failed to write data to output file");
        cleanup(image, out);
        return 1;
    }

    cleanup(image, out);
    log_info("Filesystem created successfully.");
    return 0;
} 