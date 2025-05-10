# The ACNN Filesystem (ACNN)

A work in progress filesystem written in C.

## Features

- **Directory Management:** Ability to create, find, list, and delete directory entries.
- **Superblock Verification:** Logging key filesystem information.
- **Basic File System Operations:** Sample functions for managing inodes and data blocks.

## Directory Structure

```
ACNN-v1.0.0-alpha/
├── include/        # Header files
├── src/            # C source code files
├── Makefile        # Build instructions
└── README.md       # You're looking right at me bud
```

## Building the Project

Make sure you have either [gcc](https://gcc.gnu.org/), or [Clang](https://clang.llvm.org/) installed. Then, move the project source code to your `$HOME` directory, and once inside the project directory run:

```
make
```

This will create the build directory at `$HOME/build-acnn` with the compiled executable and object files.

## Running the Filesystem

After building, run the filesystem by executing:

```
cd $HOME/build-acnn
./run-acnn <disk size>
```

For example:

```
cd $HOME/build-acnn
./run-acnn 128MB
```


