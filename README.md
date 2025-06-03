# FUSE File System with On-the-Fly File Conversion

## Overview

This project implements a FUSE (Filesystem in Userspace) that provides on-the-fly file conversion between different formats. When accessing files through this file system, they are redirected to original folder, but in addition filesystem shows possible conversions as files with zero size. When they are accessed, in original folder conversion is created and you continue working with it.

## Features

- Transparent file format conversion when accessing files;
- Extensible architecture for adding new conversion types;
- Maintains original files while presenting converted views;
- Supports all standard file operations;

## Usage

To mount the file system:

```bash
./on-the-fly_conversion -d /path/to/original/dir /path/to/mount/point
```

Or:

```bash
./on-the-fly_conversion --original-dir=/path/to/original/dir /path/to/mount/point
```

- `-d` or `--original-dir` is required parameter and should be absolute path;
- `/path/to/mount/point` is required fuse parameter;

## Extending the File System

To add new conversion types you need to implement a conversion function with the signature:
```cpp
void convert_function(const char* from_file, const char* to_file);
```

Register your conversion by adding it to the mappings array in main.cpp:
```cpp
    std::array mappings{
        // other conversion mappings
        conversion_mapping{ { "from", "to" }, convert_function };
    };
```

For example you can look at one of the implemented conversions:
+ .txt to .md (`txt2md.h`)
+ .png to .jpg (`png2jpg.h` and `stb_conversions.cpp`)
+ .jpg to .png (`jpg2png.h` and `stb_conversions.cpp`)
