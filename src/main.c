#include <stdio.h>
#include <string.h>
#include "mygit.h"

void show_help(void) {
    printf("\nMyGit - Lightweight Version Control System\n\n");

    printf("Available Commands:\n");
    printf("  mygit init                  Initialize repository\n");
    printf("  mygit add <filename>        Stage a file\n");
    printf("  mygit commit -m <message>   Commit changes\n");
    printf("  mygit status                Show repository status\n");
    printf("  mygit log                   Show commit history\n");
    printf("  mygit checkout <commit-id>  Restore a version\n");
    printf("  mygit help                  Display help\n\n");
}



int main(int argc, char *argv[]) {

if (argc < 2 || strcmp(argv[1], "help") == 0) {
    show_help();
    return 0;
}
    if (strcmp(argv[1], "init") == 0) {
        return init_repository();
    }

    else if (strcmp(argv[1], "add") == 0) {
        if (argc < 3) {
            printf("Usage: mygit add <filename>\n");
            return 1;
        }
        return add_file(argv[2]);
    }

    else if (strcmp(argv[1], "commit") == 0) {
        if (argc < 4 || strcmp(argv[2], "-m") != 0) {
            printf("Usage: mygit commit -m \"message\"\n");
            return 1;
        }
        return commit_changes(argv[3]);
    }

    else if (strcmp(argv[1], "status") == 0) {
        return show_status();
    }

    else if (strcmp(argv[1], "log") == 0) {
        return show_log();
    }

    else if (strcmp(argv[1], "checkout") == 0) {
        if (argc < 3) {
            printf("Usage: mygit checkout <commit-id>\n");
            return 1;
        }
        return checkout_version(argv[2]);
    }

    else {
        printf("Unknown command: %s\n", argv[1]);
        return 1;
    }

    return 0;
}
