Building a virtual filesystem using C
=======================================

Task
-------------------
- COMP2017 Assignment
- Mar 2019 - May 2019

Description
-------------------
In this assignment, we will be implementing our own virtual filesystem, programming a library that simulates virtual file operations on a virtual disk.

The assignment will also be tested on its performance. Hence, a multithreaded solution has to be implemented, along with the synchronisation of parallel operations to ensure that the filesystem behaves correctly.

Challenges & Learning Points
-------------------
1. Assignment problem description is difficult to understand at first
   - Draw and visualise the bytes found in the 3 read files
     - directory_table
     - file_data
     - hash_data

2. Difficult to test byte blocks in C

3. Did not use FUSE module

Files
-------------------
1. Assignment Problem Description.pdf
   - Contains the description and requirements of this assignment

2. 'code/myfilesystem.c'
  - Contains all the file system operations

3. 'code/runtest.c'
   - Contains the code to run tests on the file system operations

4. 'code/binary_files' folder
   - Contains sample data bin files used for testing

5. 'code/testcase_bin' folder
   - Contains bin files used for data comparison and testing

6. 'extra' folder
   - Contains the Filesystem in Userspace (FUSE) module in Linux
   - (optional) An extension of the assignment to enable it to behave as a real filesystem
