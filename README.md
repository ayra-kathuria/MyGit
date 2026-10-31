# MyGit – A Lightweight UNIX-Based Version Control System

## About the Project
MyGit is a basic version control system developed from scratch in C using UNIX/Linux system calls and file handling.

It allows users to initialize a repository, stage files, create commits, view commit history, check repository status, and switch between saved versions.

## Features
- Initialize a MyGit repository
- Add files to the staging area
- Commit changes with messages
- View commit history
- Check repository status
- Checkout previous versions

## Commands

| Command                            | Description                |
|------------------------------------|----------------------------|
| `mygit init`                       | Initialize a repository    |
| `mygit add <filename>`             | Stage a file               |
| `mygit commit -m "message"`        | Save a new version         |
| `mygit status`                     | Show repository status     |
| `mygit log`                        | Display commit history     |
| `mygit checkout <commit-id>`       | Restore a previous version |

## Technology Stack
- C Programming Language
- UNIX/Linux
- GCC Compiler
- Makefile
- Git and GitHub for team collaboration

## Project Structure

```text
MyGit/
├── src/
│   ├── main.c
│   ├── init.c
│   ├── add.c
│   ├── commit.c
│   ├── log.c
│   ├── status.c
│   └── checkout.c
├── include/
│   └── mygit.h
├── docs/
├── Makefile
├── README.md
└── .gitignore
```

## Team Project
Developed as a collaborative academic project by a team of four members.
