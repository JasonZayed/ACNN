#ifndef ACNNFS_H
#define ACNNFS_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <errno.h>      
#include <sys/stat.h>   
#include <sys/types.h>
#include <limits.h>

#define BLOCK_SIZE 4096
#define INODE_SIZE 128
#define BOOT_BLOCK 0
#define SUPERBLOCK_BLOCK 1
#define INODE_BITMAP_BLOCK 2
#define BLOCK_BITMAP_BLOCK 3
#define INODE_TABLE_BLOCK 4
#define MAX_FILENAME_LEN 28

#define ERR_INVALID_INODE_INDEX -1
#define ERR_NO_FREE_BLOCKS -2
#define ERR_NO_FREE_INODES -3
#define ERR_FILE_TOO_LARGE -4
#define ERR_INVALID_ARGUMENTS -5
#define ERR_FILE_WRITE_FAILED -6
#define ERR_FILE_OPEN_FAILED -7

struct superblock {
    uint32_t magic_number;
    uint32_t block_size;
    uint32_t total_blocks;
    uint32_t free_blocks;
    uint32_t total_inodes;
    uint32_t free_inodes;
    uint32_t root_inode;
    uint32_t inode_size;
    uint32_t block_bitmap;
    uint32_t inode_bitmap;
};

struct inode {
    uint32_t mode;
    uint32_t size;
    uint32_t direct_blocks[10];
    uint32_t indirect_blocks;
    uint32_t reserved[4];
};

struct dir_entry {
    uint32_t inode;
    char name[MAX_FILENAME_LEN];
};

void log_error(const char *format, ...);
void log_info(const char *format, ...);
uint32_t allocate_data_block(uint8_t *image, uint32_t total_blocks, struct superblock *sb);
void free_data_block(uint8_t *image, struct superblock *sb, uint32_t block_index);
int create_directory(uint8_t *image, struct superblock *sb, uint32_t parent_inode_idx, const char *name);
void list_directory(uint8_t *image, struct superblock *sb, uint32_t dir_inode_idx);
int delete_directory(uint8_t *image, struct superblock *sb, uint32_t dir_inode_idx);
int find_dir_entry(uint8_t *image, struct superblock *sb, uint32_t dir_inode_idx, const char *name);
int add_dir_entry(uint8_t *image, struct superblock *sb, uint32_t dir_inode_idx, uint32_t file_inode_idx, const char *name);
void read_file(uint8_t *image, struct inode *file_inode, char *buffer, size_t buffer_size);
void write_file(uint8_t *image, struct superblock *sb, struct inode *file_inode, const char *data);
void initialize_reserved_blocks(uint8_t *image, struct superblock *sb);
uint32_t allocate_inode(uint8_t *image, struct superblock *sb);

size_t parse_size(char *arg);
void cleanup(uint8_t *image, FILE *out);

#endif