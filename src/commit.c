#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include "mygit.h"

#define MAX_LINE_LEN 1024

/* Copy one file from source to destination */
static int copy_file(const char *src_path, const char *dest_path) {
    FILE *src = fopen(src_path, "rb");

    if (src == NULL) {
        perror("Error opening source file");
        return 1;
    }

    FILE *dest = fopen(dest_path, "wb");

    if (dest == NULL) {
        perror("Error creating destination file");
        fclose(src);
        return 1;
    }

    char buffer[4096];
    size_t bytes;
    int result = 0;

    while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        if (fwrite(buffer, 1, bytes, dest) != bytes) {
            perror("Error writing file");
            result = 1;
            break;
        }
    }

    if (ferror(src)) {
        perror("Error reading file");
        result = 1;
    }

    if (fclose(src) != 0) {
        result = 1;
    }

    if (fclose(dest) != 0) {
        result = 1;
    }

    return result;
}

/* Copy all regular files from one snapshot to another */
static int copy_snapshot(const char *src_dir, const char *dest_dir) {
    DIR *dir = opendir(src_dir);

    if (dir == NULL) {
        perror("Error opening previous snapshot");
        return 1;
    }

    struct dirent *entry;
    struct stat st;
    char src_path[PATH_MAX];
    char dest_path[PATH_MAX];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        int src_len = snprintf(src_path, sizeof(src_path),
                               "%s/%s", src_dir, entry->d_name);

        int dest_len = snprintf(dest_path, sizeof(dest_path),
                                "%s/%s", dest_dir, entry->d_name);

        if (src_len < 0 || (size_t)src_len >= sizeof(src_path) ||
            dest_len < 0 || (size_t)dest_len >= sizeof(dest_path)) {
            fprintf(stderr, "Error: Snapshot path is too long.\n");
            closedir(dir);
            return 1;
        }

        if (stat(src_path, &st) != 0) {
            perror("Error checking snapshot file");
            closedir(dir);
            return 1;
        }

        if (S_ISREG(st.st_mode)) {
            if (copy_file(src_path, dest_path) != 0) {
                closedir(dir);
                return 1;
            }
        }
    }

    closedir(dir);
    return 0;
}

/* Generate the next sequential commit ID */
static int generate_commit_id(char *commit_id, size_t size) {
    DIR *dir = opendir(".mygit/commits");

    if (dir == NULL) {
        perror("Error opening commits directory");
        return 1;
    }

    struct dirent *entry;
    int highest = 0;

    while ((entry = readdir(dir)) != NULL) {
        int number;
        char extra;

        if (sscanf(entry->d_name, "C%d%c", &number, &extra) == 1) {
            char expected[32];

            snprintf(expected, sizeof(expected), "C%03d", number);

            if (number > highest &&
                strcmp(expected, entry->d_name) == 0) {
                highest = number;
            }
        }
    }

    closedir(dir);

    int written = snprintf(commit_id, size, "C%03d", highest + 1);

    if (written < 0 || (size_t)written >= size) {
        fprintf(stderr, "Error generating commit ID.\n");
        return 1;
    }

    return 0;
}

/* Check whether at least one file is staged */
static int has_staged_files(void) {
    FILE *fp = fopen(".mygit/staged_files.txt", "r");

    if (fp == NULL) {
        return 0;
    }

    char line[MAX_LINE_LEN];
    int found = 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
        if (line[0] != '\n' && line[0] != '\0') {
            found = 1;
            break;
        }
    }

    fclose(fp);
    return found;
}

/* Copy all staged files into the new snapshot */
static int apply_staged_files(const char *snapshot_dir) {
    FILE *fp = fopen(".mygit/staged_files.txt", "r");

    if (fp == NULL) {
        perror("Error opening staged file list");
        return 1;
    }

    char filename[MAX_LINE_LEN];
    char src_path[PATH_MAX];
    char dest_path[PATH_MAX];

    while (fgets(filename, sizeof(filename), fp) != NULL) {
        filename[strcspn(filename, "\r\n")] = '\0';

        if (filename[0] == '\0') {
            continue;
        }

        /* Only regular files in the project root are supported */
        if (strchr(filename, '/') != NULL ||
            strcmp(filename, ".") == 0 ||
            strcmp(filename, "..") == 0) {
            fprintf(stderr, "Unsupported filename: %s\n", filename);
            fclose(fp);
            return 1;
        }

        int src_len = snprintf(src_path, sizeof(src_path),
                               ".mygit/staging/%s", filename);

        int dest_len = snprintf(dest_path, sizeof(dest_path),
                                "%s/%s", snapshot_dir, filename);

        if (src_len < 0 || (size_t)src_len >= sizeof(src_path) ||
            dest_len < 0 || (size_t)dest_len >= sizeof(dest_path)) {
            fprintf(stderr, "Error: File path is too long.\n");
            fclose(fp);
            return 1;
        }

        if (copy_file(src_path, dest_path) != 0) {
            fclose(fp);
            return 1;
        }
    }

    if (ferror(fp)) {
        fclose(fp);
        return 1;
    }

    fclose(fp);
    return 0;
}

/* Save commit information */
static int save_metadata(const char *commit_dir,
                         const char *commit_id,
                         const char *message) {
    char path[PATH_MAX];

    int written = snprintf(path, sizeof(path),
                           "%s/metadata.txt", commit_dir);

    if (written < 0 || (size_t)written >= sizeof(path)) {
        fprintf(stderr, "Error: Metadata path is too long.\n");
        return 1;
    }

    FILE *fp = fopen(path, "w");

    if (fp == NULL) {
        perror("Error creating metadata");
        return 1;
    }

    fprintf(fp, "Commit ID: %s\n", commit_id);
    fprintf(fp, "Message: %s\n", message);

    if (fclose(fp) != 0) {
        return 1;
    }

    return 0;
}

/* Update HEAD with the latest commit ID */
static int update_head(const char *commit_id) {
    FILE *fp = fopen(".mygit/HEAD", "w");

    if (fp == NULL) {
        perror("Error updating HEAD");
        return 1;
    }

    fprintf(fp, "%s\n", commit_id);

    if (fclose(fp) != 0) {
        return 1;
    }

    return 0;
}

/* Clear staging after a successful commit */
static int clear_staging(void) {
    FILE *fp = fopen(".mygit/staged_files.txt", "r");

    if (fp == NULL) {
        return 1;
    }

    char filename[MAX_LINE_LEN];
    char path[PATH_MAX];

    while (fgets(filename, sizeof(filename), fp) != NULL) {
        filename[strcspn(filename, "\r\n")] = '\0';

        if (filename[0] == '\0') {
            continue;
        }

        if (strchr(filename, '/') != NULL ||
            strcmp(filename, ".") == 0 ||
            strcmp(filename, "..") == 0) {
            fclose(fp);
            return 1;
        }

        int written = snprintf(path, sizeof(path),
                               ".mygit/staging/%s", filename);

        if (written < 0 || (size_t)written >= sizeof(path)) {
            fprintf(stderr, "Error: Staging path is too long.\n");
            fclose(fp);
            return 1;
        }

        if (remove(path) != 0 && errno != ENOENT) {
            perror("Error clearing staged file");
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);

    fp = fopen(".mygit/staged_files.txt", "w");

    if (fp == NULL) {
        perror("Error clearing staged file list");
        return 1;
    }

    fclose(fp);
    return 0;
}

/* Main commit function */
int commit_changes(const char *message) {
    struct stat st;

    char commit_id[32];
    char commit_dir[PATH_MAX];
    char snapshot_dir[PATH_MAX];
    char previous_id[32];
    char previous_snapshot[PATH_MAX];

    /* 1. Validate repository */
    if (stat(".mygit", &st) != 0 || !S_ISDIR(st.st_mode) ||
        stat(".mygit/staging", &st) != 0 || !S_ISDIR(st.st_mode) ||
        stat(".mygit/commits", &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Error: Not a valid MyGit repository.\n");
        return 1;
    }

    /* 2. Validate commit message */
    if (message == NULL || message[0] == '\0') {
        fprintf(stderr, "Error: Commit message cannot be empty.\n");
        return 1;
    }

    /* 3. Check staged files */
    if (!has_staged_files()) {
        printf("Nothing to commit. No staged files.\n");
        return 1;
    }

    /* 4. Generate commit ID */
    if (generate_commit_id(commit_id, sizeof(commit_id)) != 0) {
        return 1;
    }

    int commit_len = snprintf(commit_dir, sizeof(commit_dir),
                              ".mygit/commits/%s", commit_id);

    if (commit_len < 0 || (size_t)commit_len >= sizeof(commit_dir)) {
        fprintf(stderr, "Error: Commit path is too long.\n");
        return 1;
    }

    int snapshot_len = snprintf(snapshot_dir, sizeof(snapshot_dir),
                                "%s/snapshot", commit_dir);

    if (snapshot_len < 0 ||
        (size_t)snapshot_len >= sizeof(snapshot_dir)) {
        fprintf(stderr, "Error: Snapshot path is too long.\n");
        return 1;
    }

    /* 5. Create commit directory */
    if (mkdir(commit_dir, 0777) != 0) {
        perror("Error creating commit directory");
        return 1;
    }

    if (mkdir(snapshot_dir, 0777) != 0) {
        perror("Error creating snapshot directory");
        return 1;
    }

    /* 6. Read previous HEAD */
    FILE *head = fopen(".mygit/HEAD", "r");

    if (head == NULL) {
        perror("Error reading HEAD");
        return 1;
    }

    if (fgets(previous_id, sizeof(previous_id), head) == NULL) {
        fclose(head);
        return 1;
    }

    fclose(head);
    previous_id[strcspn(previous_id, "\r\n")] = '\0';

    /* 7. Copy previous complete snapshot */
    if (strcmp(previous_id, "NONE") != 0) {
        int previous_len = snprintf(previous_snapshot,
                                    sizeof(previous_snapshot),
                                    ".mygit/commits/%s/snapshot",
                                    previous_id);

        if (previous_len < 0 ||
            (size_t)previous_len >= sizeof(previous_snapshot)) {
            fprintf(stderr, "Error: Previous snapshot path is too long.\n");
            return 1;
        }

        if (copy_snapshot(previous_snapshot, snapshot_dir) != 0) {
            return 1;
        }
    }

    /* 8. Apply staged files */
    if (apply_staged_files(snapshot_dir) != 0) {
        fprintf(stderr, "Error applying staged files.\n");
        return 1;
    }

    /* 9. Save metadata */
    if (save_metadata(commit_dir, commit_id, message) != 0) {
        return 1;
    }

    /* 10. Update HEAD */
    if (update_head(commit_id) != 0) {
        return 1;
    }

    /* 11. Clear staging */
    if (clear_staging() != 0) {
        fprintf(stderr,
                "Warning: Commit saved, but staging could not be cleared.\n");
        return 1;
    }

    printf("Committed successfully!\n");
    printf("Commit ID: %s\n", commit_id);
    printf("Message: %s\n", message);

    return 0;
}
