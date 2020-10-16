ExrSplitter
===============

# About
Do you want to split a multi-channel OpenEXR file into different files ? This tool can help you.

# Build
In order to build you need
* CMake 3.8 +
* OpenEXR 2.2.0 +
* OpenImageIO 2.1.16 +

Then just use CMake to build it as well.

# Run
```bash
ExrSplitter <Path to multi-channel OpenEXR file>
```

It will generate several new files and each file will contain at most RGBA channels.
