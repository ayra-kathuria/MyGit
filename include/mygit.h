#ifndef MYGIT_H
#define MYGIT_H

int init_repository(void);
int add_file(const char *filename);
int commit_changes(const char *message);
int show_log(void);
int show_status(void);
int checkout_version(const char *commit_id);

#endif

