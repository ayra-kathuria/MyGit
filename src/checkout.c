#include "../include/mygit.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#define PATH_SIZE 1024
#define BUFFER_SIZE 4096

static int copy_file(const char *source, const char *destination)
{
    FILE *src = fopen(source, "rb");
    if (src == NULL)
    {
        perror("Error opening source file");
        return -1;
    }

    FILE *dest = fopen(destination, "wb");
    if (dest == NULL)
    {
        perror("Error opening destination file");
        fclose(src);
        return -1;
    }

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    int failed = 0;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src)) > 0)
    {
        if (fwrite(buffer, 1, bytes_read, dest) != bytes_read)
        {
            failed = 1;
            break;
        }
    }

    if (ferror(src))
        failed = 1;

    if (fclose(src) != 0)
        failed = 1;

    if (fclose(dest) != 0)
        failed = 1;

    if (failed)
    {
        fprintf(stderr, "Error copying file: %s\n", source);
        return -1;
    }

    return 0;
}

int checkout_version(const char *commit_id)
{
    struct stat info;
    char commit_path[PATH_SIZE];
    char snapshot_path[PATH_SIZE];
    char source_path[PATH_SIZE];
    char destination_path[PATH_SIZE];
    char head_temp[PATH_SIZE];
    char head_path[PATH_SIZE];

    /* Validate the commit ID to prevent invalid paths. */
    if (commit_id == NULL || commit_id[0] != 'C' ||
        commit_id[1] == '\0')
    {
        fprintf(stderr, "Error: Invalid commit ID.\n");
        return 1;
    }

    for (size_t i = 1; commit_id[i] != '\0'; i++)
    {
        if (commit_id[i] < '0' || commit_id[i] > '9')
        {
            fprintf(stderr, "Error: Invalid commit ID.\n");
            return 1;
        }
    }

    /* Check whether a MyGit repository exists. */
    if (stat(".mygit", &info) != 0 || !S_ISDIR(info.st_mode))
    {
        fprintf(stderr, "Error: MyGit repository not found.\n");
        return 1;
    }

    if (stat(".mygit/commits", &info) != 0 ||
        !S_ISDIR(info.st_mode))
    {
        fprintf(stderr, "Error: Commits directory not found.\n");
        return 1;
    }

    /* Construct and validate the requested commit path. */
    if (snprintf(commit_path, sizeof(commit_path),
                 ".mygit/commits/%s", commit_id)
        >= (int)sizeof(commit_path))
    {
        fprintf(stderr, "Error: Commit path is too long.\n");
        return 1;
    }

    if (stat(commit_path, &info) != 0 || !S_ISDIR(info.st_mode))
    {
        fprintf(stderr, "Error: Commit %s does not exist.\n",
                commit_id);
        return 1;
    }

    if (snprintf(snapshot_path, sizeof(snapshot_path),
                 "%s/snapshot", commit_path)
        >= (int)sizeof(snapshot_path))
    {
        fprintf(stderr, "Error: Snapshot path is too long.\n");
        return 1;
    }

    if (stat(snapshot_path, &info) != 0 || !S_ISDIR(info.st_mode))
    {
        fprintf(stderr, "Error: Snapshot directory not found.\n");
        return 1;
    }

    /* Open the snapshot directory. */
    DIR *directory = opendir(snapshot_path);

    if (directory == NULL)
    {
        perror("Error opening snapshot directory");
        return 1;
    }

    struct dirent *entry;
    int failed = 0;

    /* Restore each regular file from the snapshot. */
    while ((entry = readdir(directory)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        if (snprintf(source_path, sizeof(source_path),
                     "%s/%s", snapshot_path, entry->d_name)
            >= (int)sizeof(source_path) ||
            snprintf(destination_path, sizeof(destination_path),
                     "./%s", entry->d_name)
            >= (int)sizeof(destination_path))
        {
            fprintf(stderr, "Error: File path is too long.\n");
            failed = 1;
            break;
        }

        /* Only regular files are supported in this version. */
        if (stat(source_path, &info) != 0 || !S_ISREG(info.st_mode))
        {
            fprintf(stderr, "Error: Unsupported snapshot entry: %s\n",
                    entry->d_name);
            failed = 1;
            break;
        }

        if (copy_file(source_path, destination_path) != 0)
        {
            failed = 1;
            break;
        }
    }

    if (closedir(directory) != 0)
    {
        perror("Error closing snapshot directory");
        failed = 1;
    }

    if (failed)
    {
        fprintf(stderr,
                "Checkout failed. Some files may already have been restored.\n");
        return 1;
    }

    /* Prepare the HEAD update in a temporary file. */
    if (snprintf(head_path, sizeof(head_path), ".mygit/HEAD")
        >= (int)sizeof(head_path) ||
        snprintf(head_temp, sizeof(head_temp), ".mygit/HEAD.tmp")
        >= (int)sizeof(head_temp))
    {
        fprintf(stderr, "Error: HEAD path is too long.\n");
        return 1;
    }

    FILE *head_file = fopen(head_temp, "w");

    if (head_file == NULL)
    {
        perror("Error updating HEAD");
        return 1;
    }

    int write_failed =
        fprintf(head_file, "%s\n", commit_id) < 0;

    if (fclose(head_file) != 0)
        write_failed = 1;

    if (write_failed)
    {
        remove(head_temp);
        fprintf(stderr, "Error writing HEAD.\n");
        return 1;
    }

    if (rename(head_temp, head_path) != 0)
    {
        perror("Error replacing HEAD");
        remove(head_temp);
        return 1;
    }

    printf("Successfully checked out commit %s.\n", commit_id);
    return 0;
}