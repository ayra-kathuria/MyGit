#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include "mygit.h"

int init_repository(void) {

    // Create main repository folder
    if (mkdir(".mygit", 0777) == -1) {
        if (errno == EEXIST) {
            printf("MyGit repository already exists.\n");
            return 0;
        }

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
