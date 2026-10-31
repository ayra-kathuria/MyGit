# MyGit – Architecture and Storage Design

## 1. Working Flow

MyGit follows this workflow:

1. Initialize repository.
2. Add files to the staging area.
3. Commit staged changes.
4. View commit history.
5. Check repository status.
6. Checkout a previous commit.

## 2. Internal Storage Structure

When `mygit init` is executed, the following structure is created:

```text
.mygit/
├── staging/
├── commits/
├── HEAD
└── staged_files.txt
```

### Purpose of Each Component

- `staging/`: Stores copies of files added using `mygit add`.
- `commits/`: Stores all committed versions.
- `HEAD`: Stores the ID of the currently selected commit.
- `staged_files.txt`: Stores the names of staged files.

## 3. Commit Storage Format

Every commit will have a unique sequential ID.

Example:

```text
.mygit/
└── commits/
    ├── C001/
    │   ├── snapshot/
    │   └── metadata.txt
    └── C002/
        ├── snapshot/
        └── metadata.txt
```

Each snapshot contains the complete project state at that commit.

The metadata file stores:
- Commit ID
- Commit message
- Commit date and time

## 4. Commit Workflow

When a user executes:

`mygit commit -m "First commit"`

The system will:

1. Check whether files are staged.
2. Create a new commit ID.
3. Copy the previous snapshot if one exists.
4. Apply staged file changes.
5. Save commit metadata.
6. Update HEAD.
7. Clear the staging area.

## 5. Checkout Workflow

When a user executes:

`mygit checkout C001`

The system will:

1. Verify that the requested commit exists.
2. Restore files from that commit's snapshot.
3. Update HEAD to the selected commit ID.

## 6. Important Team Agreement

- All modules must use the same directory structure.
- Commit IDs will follow the format C001, C002, C003, etc.
- Every commit stores a complete snapshot.
- `HEAD` stores the currently selected commit ID.
- `staged_files.txt` stores one filename per line.
- Initial version supports regular files in the project root.
- Nested directories and file deletion are outside the initial scope.

B
B
B
B
B
B
B
B
B
B
B
B
B
B
B
B
B
B
B
B
B
B
B
B
B
B
B
B
B
B
B
B
- Real Git is used only for collaboration, not for MyGit's internal version control.
