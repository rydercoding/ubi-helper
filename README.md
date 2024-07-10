## Overview
This tool can analyze the ubifs image file, which is dumped from the device or generated from ubifs generation tool such as mkfs.ubifs. There are 2 usages for this tool:
1. Dump the superblock node, master node including their fileds. (More types of nodes dumpping can easily added in the code)
2. Corrupt the ubifs image file. It's mostly used for constructing some special test cases by using some corrupted ubifs image.

## How to build the code
$ make clean

$ make

## How to use the tool
At this moment, the user has to customize the code and build the code, as some settings are hard-coded:
- UBIFS_IN_FILE_PATH should be changed to the full path of file to be dumped.
- UBIFS_CORRUPT_FILE_PATH should be changed to the full path of file to be corrupted.
- LEB_SIZE should be changed to the actual LEB block size. It's mostly the same size to the block size of the used NAND chip.

To dump the ubifs file:
$ ./uib-helper -d
To corrupt the ubifs file:
$ ./uib-helper -c
