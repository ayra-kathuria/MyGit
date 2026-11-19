#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#include "mygit.h"

#define MAX_LINE_LEN 1024

static int is_staged(const char *filename) {
    FILE *fp = fopen(".mygit/staged_files.txt", "r");
    if (fp == NULL) return 0;

    char line[MAX_LINE_LEN];
    int found = 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
        line[strcspn(line, "\r\n")] = '\0';
        if (strcmp(line, filename) == 0) {
            found = 1;
            break;
        }
    }

    fclose(fp);
    return found;
}

static int files_differ(const char *file1, const char *file2) {
    FILE *fp1 = fopen(file1, "rb");
    FILE *fp2 = fopen(file2, "rb");

    if (fp1 == NULL || fp2 == NULL) {
        if (fp1 != NULL) fclose(fp1);
        if (fp2 != NULL) fclose(fp2);
        return 1;
    }

    int ch1, ch2;
    int different = 0;

    do {
        ch1 = fgetc(fp1);
        ch2 = fgetc(fp2);

        if (ch1 != ch2) {
            different = 1;
            break;
        }
    } while (ch1 != EOF && ch2 != EOF);

    fclose(fp1);
    fclose(fp2);
    return different;
}

int show_status(void) {
    struct stat st;
    char head_id[32];
    char snapshot_dir[PATH_MAX];

    if (stat(".mygit", &st) != 0 || !S_ISDIR(st.st_mode) ||
        stat(".mygit/staging", &st) != 0 ||
        stat(".mygit/commits", &st) != 0 ||
        stat(".mygit/HEAD", &st) != 0 ||
        stat(".mygit/staged_files.txt", &st) != 0) {
        printf("Error: Not a valid MyGit repository.\n");
        return 1;
    }

    FILE *head = fopen(".mygit/HEAD", "r");
    if (head == NULL) {
        perror("Error reading HEAD");
        return 1;
    }

    if (fgets(head_id, sizeof(head_id), head) == NULL) {
        fclose(head);
        return 1;
    }

    fclose(head);
    head_id[strcspn(head_id, "\r\n")] = '\0';

    printf("\n------ MyGit Status ------\n");
    printf("Current commit: %s\n\n", head_id);

    if (strcmp(head_id, "NONE") != 0) {
        snprintf(snapshot_dir, sizeof(snapshot_dir),
                 ".mygit/commits/%s/snapshot", head_id);

        if (stat(snapshot_dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
            printf("Error: Current commit snapshot not found.\n");
            return 1;
        }
    } else {
        strcpy(snapshot_dir, "");
    }

    DIR *dir = opendir(".");
    if (dir == NULL) {
        perror("Error opening working directory");
        return 1;
    }

    struct dirent *entry;
    char current_path[PATH_MAX];
    char saved_path[PATH_MAX];

    int staged_count = 0;
    int modified_count = 0;
    int untracked_count = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0 ||
            strcmp(entry->d_name, ".mygit") == 0) {
            continue;
        }

        if (snprintf(current_path, sizeof(current_path),
                     "./%s", entry->d_name) >= sizeof(current_path)) {
            continue;
        }

        if (stat(current_path, &st) != 0 || !S_ISREG(st.st_mode)) {
            continue;
        }

        if (is_staged(entry->d_name)) {
            printf("[STAGED] %s\n", entry->d_name);
            staged_count++;
        }
        else if (strcmp(head_id, "NONE") == 0) {
            printf("[UNTRACKED] %s\n", entry->d_name);
            untracked_count++;
        }
        else {
            if (snprintf(saved_path, sizeof(saved_path),
                         "%s/%s", snapshot_dir,
                         entry->d_name) >= sizeof(saved_path)) {
                continue;
            }

            if (stat(saved_path, &st) != 0) {
                printf("[UNTRACKED] %s\n", entry->d_name);
                untracked_count++;
            }
            else if (files_differ(current_path, saved_path)) {
                printf("[MODIFIED] %s\n", entry->d_name);
                modified_count++;
            }
        }
    }

    closedir(dir);

    if (staged_count == 0 && modified_count == 0 &&
        untracked_count == 0) {
        printf("Working directory is clean.\n");
    }

    printf("\nStaged: %d | Modified: %d | Untracked: %d\n",
           staged_count, modified_count, untracked_count);

    return 0;
}