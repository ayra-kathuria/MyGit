#include "mygit.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>

#define MAX_COMMITS 10000

int show_log(void)
{
    DIR *dir = opendir(".mygit/commits");
    struct dirent *entry;
    int ids[MAX_COMMITS];
    int count = 0;

    if (dir == NULL) {
        printf("Repository not initialized.\n");
        return 1;
    }

    while ((entry = readdir(dir)) != NULL) {
        int id;
        char expected[32];

        if (sscanf(entry->d_name, "C%d", &id) == 1 && id > 0) {
            snprintf(expected, sizeof(expected), "C%03d", id);

            if (strcmp(entry->d_name, expected) == 0 &&
                count < MAX_COMMITS) {
                ids[count++] = id;
            }
        }
    }

    closedir(dir);

    if (count == 0) {
        printf("No commits found.\n");
        return 0;
    }

    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (ids[i] < ids[j]) {
                int temp = ids[i];
                ids[i] = ids[j];
                ids[j] = temp;
            }
        }
    }

    printf("\n========== MyGit Commit History ==========\n");

    for (int i = 0; i < count; i++) {
        char path[1024];
        char line[1024];

        snprintf(path, sizeof(path),
                 ".mygit/commits/C%03d/metadata.txt", ids[i]);

        FILE *fp = fopen(path, "r");

        if (fp == NULL) {
            printf("Metadata unavailable for C%03d\n", ids[i]);
            continue;
        }

        printf("\n");

        while (fgets(line, sizeof(line), fp) != NULL) {
            printf("%s", line);
        }

        fclose(fp);
        printf("------------------------------------------\n");
    }

    return 0;
}
