#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include "mygit.h"

int init_repository(void) {

    struct stat st;

    // Check whether repository already exists
    if (stat(".mygit", &st) == 0) {

        if (!S_ISDIR(st.st_mode)) {
            printf("Error: .mygit exists but is not a directory.\n");
            return 1;
        }

        // Verify existing repository structure
        if (stat(".mygit/staging", &st) == 0 &&
            S_ISDIR(st.st_mode) &&
            stat(".mygit/commits", &st) == 0 &&
            S_ISDIR(st.st_mode) &&
            stat(".mygit/HEAD", &st) == 0 &&
            S_ISREG(st.st_mode) &&
            stat(".mygit/staged_files.txt", &st) == 0 &&
            S_ISREG(st.st_mode)) {

            printf("MyGit repository already exists.\n");
            return 0;
        }

        printf("Error: Repository structure is incomplete.\n");
        return 1;
    }

    // Handle errors other than repository not existing
    if (errno != ENOENT) {
        perror("Error checking repository");
        return 1;
    }

    // Create main repository directory
    if (mkdir(".mygit", 0777) == -1) {
        perror("Error creating repository");
        return 1;
    }

    // Create staging directory
    if (mkdir(".mygit/staging", 0777) == -1) {
        perror("Error creating staging directory");
        return 1;
    }

    // Create commits directory
    if (mkdir(".mygit/commits", 0777) == -1) {
        perror("Error creating commits directory");
        return 1;
    }

    // Create HEAD file
    FILE *head = fopen(".mygit/HEAD", "w");

    if (head == NULL) {
        perror("Error creating HEAD");
        return 1;
    }

    fprintf(head, "NONE");
    fclose(head);

    // Create staged files list
    FILE *staged = fopen(".mygit/staged_files.txt", "w");

    if (staged == NULL) {
        perror("Error creating staged_files.txt");
        return 1;
    }

    fclose(staged);

    printf("Initialized empty MyGit repository successfully!\n");

    return 0;
}
