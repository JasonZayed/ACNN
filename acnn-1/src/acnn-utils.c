#include "../include/acnn.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

void log_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "[ERROR] ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}

void log_info(const char *format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stdout, "[INFO] ");
    vfprintf(stdout, format, args);
    fprintf(stdout, "\n");
    va_end(args);
}

void handle_error(int error_code, const char *message) {
    switch (error_code) {
        case ERR_INVALID_INODE_INDEX:
            log_error("Invalid inode index: %s", message);
            break;
        case ERR_NO_FREE_BLOCKS:
            log_error("No free blocks available: %s", message);
            break;
        case ERR_NO_FREE_INODES:
            log_error("No free inodes available: %s", message);
            break;
        case ERR_FILE_TOO_LARGE:
            log_error("File too large: %s", message);
            break;
        default:
            log_error("Unknown error: %s", message);
            break;
    }
}

void cleanup(uint8_t *image, FILE *out) {
    if (image) {
        free(image);
        log_info("Memory freed successfully");
    }
    if (out) {
        fclose(out);
        log_info("File closed successfully");
    }
}

size_t parse_size(char *arg) {
    size_t size = atoi(arg);
    if (strstr(arg, "MB")) size *= 1024 * 1024;
    else if (strstr(arg, "KB")) size *= 1024;
    return size;
}