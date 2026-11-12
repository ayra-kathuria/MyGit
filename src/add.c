#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "mygit.h"

#define MAX_PATH_LEN 1024
#define MAX_LINE_LEN 256

/* Helper: check if a directory exists using POSIX stat() */
static int directory_exists(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
        return 1;
    }
    return 0;
}

/* Helper: copy source file into destination path in binary mode */
static int copy_file_to_staging(const char *src_path, const char *dest_path) {
    FILE *src = fopen(src_path, "rb");
    if (src == NULL) {
        perror("Error opening source file");
        return -1;
    }

    FILE *dest = fopen(dest_path, "wb");
    if (dest == NULL) {
        perror("Error creating staged file in .mygit/staging");
        fclose(src);
        return -1;
    }

    char buffer[4096];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        if (fwrite(buffer, 1, bytes_read, dest) != bytes_read) {
            perror("Error writing to staged file");
            fclose(src);
            fclose(dest);
            return -1;
        }
    }

    if (ferror(src)) {
        perror("Error reading source file");
        fclose(src);
        fclose(dest);
        return -1;
    }

    fclose(src);
    fclose(dest);
    return 0;
}

/* Helper: check if filename is already listed in .mygit/staged_files.txt */
static int is_file_already_staged(const char *filename) {
    FILE *fp = fopen(".mygit/staged_files.txt", "r");
    if (fp == NULL) {
        return 0;
    }

    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), fp) != NULL) {
        line[strcspn(line, "\r\n")] = '\0';
        if (strcmp(line, filename) == 0) {
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    return 0;
}

int add_file(const char *filename) {
    /* 1. Validate that a filename string was provided */
    if (filename == NULL || strlen(filename) == 0) {
        fprintf(stderr, "Error: No filename specified.\n");
        return 1;
    }

    /* 2. Check whether the current directory contains a valid .mygit repository */
    if (!directory_exists(".mygit") || !directory_exists(".mygit/staging")) {
        fprintf(stderr, "Error: Not a MyGit repository (run 'mygit init' first).\n");
        return 1;
    }

    /* 3. Check whether the specified file exists in the working directory */
    struct stat file_stat;
    if (stat(filename, &file_stat) != 0) {
        fprintf(stderr, "Error: File '%s' does not exist.\n", filename);
        return 1;
    }

    /* 4. Ensure the specified path is a regular file (not a directory) */
    if (!S_ISREG(file_stat.st_mode)) {
        fprintf(stderr, "Error: '%s' is not a regular file.\n", filename);
        return 1;
    }

    /* 5. Copy the file into .mygit/staging/<filename> */
    char staging_path[MAX_PATH_LEN];
    int written = snprintf(staging_path, sizeof(staging_path), ".mygit/staging/%s", filename);
    if (written < 0 || (size_t)written >= sizeof(staging_path)) {
        fprintf(stderr, "Error: Filename '%s' is too long.\n", filename);
        return 1;
    }

    if (copy_file_to_staging(filename, staging_path) != 0) {
        fprintf(stderr, "Error: Failed to copy '%s' to staging area.\n", filename);
        return 1;
    }

    /* 6. Avoid duplicate entries in .mygit/staged_files.txt */
    if (is_file_already_staged(filename)) {
        printf("Updated staged copy of '%s' (already listed in staged_files.txt).\n", filename);
        return 0;
    }

    /* 7. Append the new filename to .mygit/staged_files.txt */
    FILE *staged_fp = fopen(".mygit/staged_files.txt", "a");
    if (staged_fp == NULL) {
        perror("Error opening .mygit/staged_files.txt");
        return 1;
    }

    fprintf(staged_fp, "%s\n", filename);
    fclose(staged_fp);

    printf("Staged '%s' successfully.\n", filename);
    return 0;
}
