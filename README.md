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
1. Data processing
   - gene expression matrices
     - convert Ensembl IDs to official gene symbols
     - joined multiple matrices by common gene symbols
     - transformations (e.g. log~2) and normalisations
   - CEL files - into a gene expression matrix

2. Model selection
   - use of penalised logistic regression methods to account for the large p small n situation
   - use of the Brier Score as metric to evaluate models for both Part 1 and Part 3 as a way to validate the results from the AUC

3. Shiny application
   - Designing the interface
   - Reading in raw files
   - Create alternative models on the spot if the input data is not suitable for the trained model (e.g. genes from input patient data does not match filtered genes used in the trained model)

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
