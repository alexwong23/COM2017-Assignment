#include <stdio.h>
#include <string.h>

#define TEST(x) test(x, #x)
// #include "myfilesystem.h" // comment out for local machine
#include "myfilesystem.c" // include for local machine

/* You are free to modify any part of this file. The only requirement is that when it is run, all your tests are automatically executed */

// typedef struct real_files { // comment out for local machine
// 	FILE * f1;
// 	FILE * f2;
// 	FILE * f3;
// 	int num_files;
// } real_files;

void duplicate_file(char* filename, char* destname) {
  FILE* fp = fopen(filename, "rb");
  if (fp == NULL) {
      perror("Could not find sample file\n");
      exit(1); // comment out for edstem
   }
   FILE* dest = fopen(destname, "wb");
   if (dest == NULL) {
      fclose(fp);
      perror("Could not create destination file\n");
      exit(1); // comment out for edstem
   }
   char ch;
   while ((ch = fgetc(fp)) != EOF)
      fputc(ch, dest);

   fclose(fp);
   fclose(dest);
}

void duplicate_sample_bin_files(void) {
  duplicate_file("./binary_files/sample-part1-only_file_data.bin", "./binary_files/dup_file_data.bin");
  duplicate_file("./binary_files/sample-part1-only_directory_table.bin", "./binary_files/dup_dir_table.bin");
  duplicate_file("./binary_files/sample-part1-only_hash_data.bin", "./binary_files/dup_hash_data.bin");
}

int compare_binary_files(const char* output_filename, FILE* expected) {
  FILE* output_file = fopen(output_filename, "r");
  char char1 = fgetc(expected);
  char char2 = fgetc(output_file);
  int pos_error = 0;
  int error = 0;

  while(char1 != EOF && char2 != EOF) {
    if(char1 != char2 && char1 == '\0' && char2 == '\0') {
      error = 1;
      printf("File %s compare error @ byte %d: output %c expected %c\n", output_filename, pos_error, char1, char2);
      break;
    }
    pos_error++;
    char1 = fgetc(expected);
    char2 = fgetc(output_file);
  }
  return error;
}

int compare_helper_files(void* helper, const char* file1, const char* file2, const char* file3) {
  real_files* r_files = (real_files*) helper;
  if(compare_binary_files(file1, r_files->f1) || compare_binary_files(file2, r_files->f2) || compare_binary_files(file3, r_files->f3) ) {
    return 1;
  }
  return 0;
}

// Remember you need to provide your own test files and also check their contents as part of testing

int read_bin_file() {
  duplicate_sample_bin_files();
  void* helper = init_fs("./binary_files/dup_file_data.bin", "./binary_files/dup_dir_table.bin", "./binary_files/dup_hash_data.bin", 1);
  real_files* r_files = (real_files*) helper;
  print_sample_file_contents(r_files->f1, "dup_file_data");
  print_sample_file_contents(r_files->f2, "dup_dir_table");
  // print_sample_file_contents(r_files->f3, "dup_hash_data");
  if(helper) {
		close_fs(helper);
		return 0;
	} else {
		close_fs(helper);
		return 1;
	}
}

int init_null_file_fail() {
  void * helper = init_fs("dup_file_data.bin", "dup_dir_table.bin", NULL, 1);
	if(helper) {
		close_fs(helper);
		return 1;
	} else {
		close_fs(helper);
		return 0;
	}
}

int init_file_not_found_fail() {
  void * helper = init_fs("test", "dup_dir_table.bin", "dup_hash_data.bin", 1);
	if(helper) {
		close_fs(helper);
		return 1;
	} else {
		close_fs(helper);
		return 0;
	}
}

int init_simple_success() {
  duplicate_sample_bin_files();
  void* helper = init_fs("./binary_files/dup_file_data.bin", "./binary_files/dup_dir_table.bin", "./binary_files/dup_hash_data.bin", 1);
	if(helper) {
    if(compare_helper_files(helper, "./testcase_bin/00_init_simple_success_f1.bin",
        "./testcase_bin/00_init_simple_success_f2.bin", "./testcase_bin/00_init_simple_success_f3.bin")) {
      close_fs(helper);
      return 1;
    }
    close_fs(helper);
		return 0;
	} else {
		close_fs(helper);
		return 1;
	}
}

int create_file_null_argument_failure() {
  duplicate_sample_bin_files();
  void* helper = init_fs("./binary_files/dup_file_data.bin", "./binary_files/dup_dir_table.bin", "./binary_files/dup_hash_data.bin", 1);
  int rtn_value = create_file("testfilename", 15, NULL);
	if(rtn_value) {
		close_fs(helper);
		return 0;
	} else {
		close_fs(helper);
		return 1;
	}
}

int create_file_null_byte_filename_failure() {
  duplicate_sample_bin_files();
  void* helper = init_fs("./binary_files/dup_file_data.bin", "./binary_files/dup_dir_table.bin", "./binary_files/dup_hash_data.bin", 1);
  int rtn_value = create_file("\0", 15, helper);
	if(rtn_value) {
		close_fs(helper);
		return 0;
	} else {
		close_fs(helper);
		return 1;
	}
}

// FIX truncate will never error if exceed length
int create_file_truncate() {
  duplicate_sample_bin_files();
  void* helper = init_fs("./binary_files/dup_file_data.bin", "./binary_files/dup_dir_table.bin", "./binary_files/dup_hash_data.bin", 1);
  if(create_file("0123456789|123456789|123456789|123456789|123456789|123456789|123456789", 15, helper)) {
		close_fs(helper);
		return 1;
	}
  if(create_file("virtual_file", 10, helper)) {
		close_fs(helper);
		return 1;
	}
  if(compare_helper_files(helper, "./testcase_bin/01_create_truncate_f1.bin",
      "./testcase_bin/01_create_truncate_f1.bin", "./testcase_bin/01_create_truncate_f1.bin")) {
    close_fs(helper);
    return 1;
  }
  close_fs(helper);
	return 0;
}

int create_file_duplicate_filename_failure() {
  duplicate_sample_bin_files();
  void* helper = init_fs("./binary_files/dup_file_data.bin", "./binary_files/dup_dir_table.bin", "./binary_files/dup_hash_data.bin", 1);
  if(create_file("Document.docx", 30, helper)) {
		close_fs(helper);
		return 0;
	}
	close_fs(helper);
	return 1;
}

int create_file_simple_success() {
  duplicate_sample_bin_files();
  void* helper = init_fs("./binary_files/dup_file_data.bin", "./binary_files/dup_dir_table.bin", "./binary_files/dup_hash_data.bin", 1);
  if(create_file("virtual_file", 500, helper)) {
		close_fs(helper);
		return 1;
	}
  if(create_file("second file here", 30, helper)) {
		close_fs(helper);
		return 1;
	}
  if(compare_helper_files(helper, "./testcase_bin/01_create_truncate_f1.bin",
      "./testcase_bin/01_create_truncate_f1.bin", "./testcase_bin/01_create_truncate_f1.bin")) {
    close_fs(helper);
    return 1;
  }
  close_fs(helper);
	return 0;
}

int create_file_skip_null_success() {
  duplicate_sample_bin_files();
  void* helper = init_fs("./binary_files/dup_file_data.bin", "./binary_files/dup_dir_table.bin", "./binary_files/dup_hash_data.bin", 1);
  if(create_file("virtual_file", 500, helper)) {
		close_fs(helper);
		return 1;
	}
  if(create_file("\0", 15, helper) == 0) { // string terminator fail
		close_fs(helper);
		return 1;
	}
  if(create_file("second file here", 30, helper)) {
		close_fs(helper);
		return 1;
	}
	close_fs(helper);
	return 0;
}

int resize_file_does_not_exist_fail() {
  duplicate_sample_bin_files();
  void* helper = init_fs("./binary_files/dup_file_data.bin", "./binary_files/dup_dir_table.bin", "./binary_files/dup_hash_data.bin", 1);
  if(create_file("virtual_file", 500, helper)) {
    close_fs(helper);
    return 1;
  }
  if(create_file("second file here", 30, helper)) {
    close_fs(helper);
    return 1;
  }
  int resize_rtn = resize_file("not found", 20, helper);
  if(resize_rtn == 1) {
    close_fs(helper);
    return 0;
  }
  close_fs(helper);
  return 1;
}

int resize_file_insufficient_space_fail() {
  duplicate_sample_bin_files();
  void* helper = init_fs("./binary_files/dup_file_data.bin", "./binary_files/dup_dir_table.bin", "./binary_files/dup_hash_data.bin", 1);
  if(create_file("virtual_file", 50000, helper)) {
    close_fs(helper);
    return 0;
  }
  if(create_file("second file here", 30, helper)) {
    close_fs(helper);
    return 1;
  }
  close_fs(helper);
  return 1;
}

int resize_file_decrease_size_success() {
  duplicate_sample_bin_files();
  void* helper = init_fs("./binary_files/dup_file_data.bin", "./binary_files/dup_dir_table.bin", "./binary_files/dup_hash_data.bin", 1);
  if(create_file("virtual_file", 100, helper)) {
    close_fs(helper);
    return 1;
  }
  if(create_file("second file here", 60, helper)) {
    close_fs(helper);
    return 1;
  }
  int resize_rtn = resize_file("second file here", 30, helper);
  if(resize_rtn == 0) {
    close_fs(helper);
    return 0;
  }
  close_fs(helper);
  return 1;
}

int resize_file_increase_size_success() {
  duplicate_sample_bin_files();
  void* helper = init_fs("./binary_files/dup_file_data.bin", "./binary_files/dup_dir_table.bin", "./binary_files/dup_hash_data.bin", 1);
  if(create_file("virtual_file", 500, helper)) {
    close_fs(helper);
    return 1;
  }
  if(create_file("second file here", 30, helper)) {
    close_fs(helper);
    return 1;
  }
  int resize_rtn = resize_file("second file here", 90, helper);
  if(resize_rtn == 0) {
    close_fs(helper);
    return 0;
  }
  close_fs(helper);
  return 1;
}

int repack_file_success() {
  duplicate_sample_bin_files();
  void* helper = init_fs("./binary_files/dup_file_data.bin", "./binary_files/dup_dir_table.bin", "./binary_files/dup_hash_data.bin", 1);
  if(create_file("virtual_file", 500, helper)) {
    close_fs(helper);
    return 1;
  }
  if(create_file("second file here", 30, helper)) {
    close_fs(helper);
    return 1;
  }
  int resize_rtn = resize_file("second file here", 91, helper);
  if(resize_rtn == 0) {
    close_fs(helper);
    return 0;
  }
  close_fs(helper);
  return 1;
}

int repack_file_insufficient_space_fail() {
  duplicate_sample_bin_files();
  void* helper = init_fs("./binary_files/dup_file_data.bin", "./binary_files/dup_dir_table.bin", "./binary_files/dup_hash_data.bin", 1);
  if(create_file("virtual_file", 500, helper)) {
    close_fs(helper);
    return 1;
  }
  if(create_file("second file here", 30, helper)) {
    close_fs(helper);
    return 1;
  }
  int resize_rtn = resize_file("second file here", 5000, helper);
  if(resize_rtn == 2) {
    close_fs(helper);
    return 0;
  }
  close_fs(helper);
  return 1;
}

int delete_file_NULL_failure() {
  duplicate_sample_bin_files();
  void* helper = init_fs("./binary_files/dup_file_data.bin", "./binary_files/dup_dir_table.bin", "./binary_files/dup_hash_data.bin", 1);
  if(create_file("virtual_file", 500, helper)) {
    close_fs(helper);
    return 1;
  }
  int del_value = delete_file("file not exist", NULL);
	if(del_value) {
		close_fs(helper);
		return 0;
	}
  close_fs(helper);
  return 1;
}

int delete_file_not_found_failure() {
  duplicate_sample_bin_files();
  void* helper = init_fs("./binary_files/dup_file_data.bin", "./binary_files/dup_dir_table.bin", "./binary_files/dup_hash_data.bin", 1);
  if(create_file("virtual_file", 500, helper)) {
    close_fs(helper);
    return 1;
  }
  int del_value = delete_file("file not exist", helper);
	if(del_value) {
		close_fs(helper);
		return 0;
	}
  close_fs(helper);
  return 1;
}

int delete_file_success() {
  duplicate_sample_bin_files();
  void* helper = init_fs("./binary_files/dup_file_data.bin", "./binary_files/dup_dir_table.bin", "./binary_files/dup_hash_data.bin", 1);
  if(create_file("virtual_file", 500, helper)) {
    close_fs(helper);
    return 1;
  }
  if(create_file("second file here", 30, helper)) {
    close_fs(helper);
    return 1;
  }
  int del_value = delete_file("virtual_file", helper);
	if(del_value) {
		close_fs(helper);
		return 1;
	}
  close_fs(helper);
  return 0;
}

int rename_file_not_found_fail() {
  duplicate_sample_bin_files();
  void* helper = init_fs("./binary_files/dup_file_data.bin", "./binary_files/dup_dir_table.bin", "./binary_files/dup_hash_data.bin", 1);
  if(create_file("virtual_file", 500, helper)) {
    close_fs(helper);
    return 1;
  }
  if(create_file("second file here", 30, helper)) {
    close_fs(helper);
    return 1;
  }
  int ren_value = rename_file("virtuals_file", "please rename", helper);
  if(ren_value == 1) {
    close_fs(helper);
    return 0;
  }
  close_fs(helper);
  return 1;
}

int rename_file_duplicate_found_fail() {
  duplicate_sample_bin_files();
  void* helper = init_fs("./binary_files/dup_file_data.bin", "./binary_files/dup_dir_table.bin", "./binary_files/dup_hash_data.bin", 1);
  if(create_file("virtual_file", 500, helper)) {
    close_fs(helper);
    return 1;
  }
  if(create_file("second file here", 30, helper)) {
    close_fs(helper);
    return 1;
  }
  int ren_value = rename_file("virtual_file", "second file here", helper);
  if(ren_value == 1) {
    close_fs(helper);
    return 0;
  }
  close_fs(helper);
  return 1;
}

int rename_file_success() {
  duplicate_sample_bin_files();
  void* helper = init_fs("./binary_files/dup_file_data.bin", "./binary_files/dup_dir_table.bin", "./binary_files/dup_hash_data.bin", 1);
  if(create_file("virtual.txt", 500, helper)) {
    close_fs(helper);
    return 1;
  }
  if(create_file("second file here", 30, helper)) {
    close_fs(helper);
    return 1;
  }
  int ren_value = rename_file("virtual.txt", "virtual.c", helper);
  if(ren_value == 0) {
    close_fs(helper);
    return 0;
  }
  close_fs(helper);
  return 1;
}

int read_file_not_found_fail() {
  duplicate_sample_bin_files();
  void* helper = init_fs("./binary_files/dup_file_data.bin", "./binary_files/dup_dir_table.bin", "./binary_files/dup_hash_data.bin", 1);
  if(create_file("virtual_file", 500, helper)) {
    close_fs(helper);
    return 1;
  }
  if(create_file("second file here", 30, helper)) {
    close_fs(helper);
    return 1;
  }
  char buffer[100];
  int read_value = read_file("virtuals_file", 0, 10, buffer, helper);
  if(read_value == 1) {
    close_fs(helper);
    return 0;
  }
  close_fs(helper);
  return 1;
}

int read_file_overflow_fail() {
  duplicate_sample_bin_files();
  void* helper = init_fs("./binary_files/dup_file_data.bin", "./binary_files/dup_dir_table.bin", "./binary_files/dup_hash_data.bin", 1);
  if(create_file("virtual_file", 500, helper)) {
    close_fs(helper);
    return 1;
  }
  if(create_file("second file here", 30, helper)) {
    close_fs(helper);
    return 1;
  }
  char buffer[100];
  int read_value = read_file("hello", 0, 48, buffer, helper);
  if(read_value == 2) {
    close_fs(helper);
    return 0;
  }
  close_fs(helper);
  return 1;
}

int read_file_simple_success() {
  duplicate_sample_bin_files();
  void* helper = init_fs("./binary_files/dup_file_data.bin", "./binary_files/dup_dir_table.bin", "./binary_files/dup_hash_data.bin", 1);
  if(create_file("virtual_file", 500, helper)) {
    close_fs(helper);
    return 1;
  }
  if(create_file("second file here", 30, helper)) {
    close_fs(helper);
    return 1;
  }
  char buffer[100];
  int read_value = read_file("hello", 0, 47, buffer, helper);
  if(read_value == 0) {
    printf("read %s\n", buffer);
    close_fs(helper);
    return 0;
  }
  close_fs(helper);
  return 1;
}

int write_file_not_found_fail() {
  duplicate_sample_bin_files();
  void* helper = init_fs("./binary_files/dup_file_data.bin", "./binary_files/dup_dir_table.bin", "./binary_files/dup_hash_data.bin", 1);
  if(create_file("virtual_file", 500, helper)) {
    close_fs(helper);
    return 1;
  }
  if(create_file("second file here", 30, helper)) {
    close_fs(helper);
    return 1;
  }
  char buffer[100];
  if(read_file("hello", 0, 47, buffer, helper) != 0) {
    close_fs(helper);
    return 1;
  }
  int write_value = write_file("virtuals_file", 0, 10, buffer, helper);
  if(write_value == 1) {
    close_fs(helper);
    return 0;
  }
  close_fs(helper);
  return 1;
}

int write_file_offset_greater_fail() {
  duplicate_sample_bin_files();
  void* helper = init_fs("./binary_files/dup_file_data.bin", "./binary_files/dup_dir_table.bin", "./binary_files/dup_hash_data.bin", 1);
  if(create_file("virtual_file", 500, helper)) {
    close_fs(helper);
    return 1;
  }
  if(create_file("second file here", 30, helper)) {
    close_fs(helper);
    return 1;
  }
  char buffer[100];
  if(read_file("hello", 0, 47, buffer, helper) != 0) {
    close_fs(helper);
    return 1;
  }
  int write_value = write_file("second file here", 31, 30, buffer, helper);
  if(write_value == 2) {
    close_fs(helper);
    return 0;
  }
  close_fs(helper);
  return 1;
}

int write_file_insufficient_space_fail() {
  duplicate_sample_bin_files();
  void* helper = init_fs("./binary_files/dup_file_data.bin", "./binary_files/dup_dir_table.bin", "./binary_files/dup_hash_data.bin", 1);
  if(create_file("virtual_file", 500, helper)) {
    close_fs(helper);
    return 1;
  }
  if(create_file("second file here", 30, helper)) {
    close_fs(helper);
    return 1;
  }
  char buffer[100];
  if(read_file("hello", 0, 47, buffer, helper) != 0) {
    close_fs(helper);
    return 1;
  }
  int write_value = write_file("second file here", 19, 5000, buffer, helper);
  if(write_value == 3) {
    close_fs(helper);
    return 0;
  }
  close_fs(helper);
  return 1;
}

int write_file_simple_success() {
  duplicate_sample_bin_files();
  void* helper = init_fs("./binary_files/dup_file_data.bin", "./binary_files/dup_dir_table.bin", "./binary_files/dup_hash_data.bin", 1);
  if(create_file("virtual_file", 500, helper)) {
    close_fs(helper);
    return 1;
  }
  if(create_file("second file here", 30, helper)) {
    close_fs(helper);
    return 1;
  }
  char buffer[100];
  if(read_file("hello", 0, 47, buffer, helper) != 0) {
    close_fs(helper);
    return 1;
  }
  int write_value = write_file("second file here", 0, 5, buffer, helper);
  if(write_value == 0) {
    close_fs(helper);
    return 0;
  }
  close_fs(helper);
  return 1;
}

int write_file_repack_success() {
  duplicate_sample_bin_files();
  void* helper = init_fs("./binary_files/dup_file_data.bin", "./binary_files/dup_dir_table.bin", "./binary_files/dup_hash_data.bin", 1);
  if(create_file("virtual_file", 500, helper)) {
    close_fs(helper);
    return 1;
  }
  if(create_file("second file here", 30, helper)) {
    close_fs(helper);
    return 1;
  }
  char buffer[100];
  if(read_file("Document.docx", 0, 5, buffer, helper) != 0) {
    close_fs(helper);
    return 1;
  }
  if(write_file("second file here", 0, 5, buffer, helper) != 0) {
    close_fs(helper);
    return 1;
  }

  char buffer2[100];
  if(read_file("second file here", 0, 4, buffer2, helper) != 0) {
    close_fs(helper);
    return 1;
  }
  if(write_file("hello", 47, 47, buffer2, helper) != 0) {
    close_fs(helper);
    return 1;
  }
  char buffer3[100];
  if(read_file("hello", 0, 94, buffer3, helper) != 0) {
    close_fs(helper);
    return 1;
  }
  int write_value = write_file("Document.docx", 0, 94, buffer3, helper);
  if(write_value == 0) {
    close_fs(helper);
    return 0;
  }
  close_fs(helper);
  return 1;
}

int compute_hash_block_test() {
  duplicate_sample_bin_files();
  void* helper = init_fs("./binary_files/dup_file_data.bin", "./binary_files/dup_dir_table.bin", "./binary_files/dup_hash_data.bin", 1);
  compute_hash_block(0, helper);
  close_fs(helper);
  return 0;
}

/****************************/

/* Helper function */
void test(int (*test_function) (), char * function_name) {
    int ret = test_function();
    if (ret == 0) {
        printf("Passed %s\n", function_name);
    } else {
        printf("\n*****FAILED*****\n %s returned %d\n", function_name, ret);
    }
}
/************************/

int main(int argc, char * argv[]) {

    // You can use the TEST macro as TEST(x) to run a test function named "x"

  duplicate_sample_bin_files();
  // TEST(read_bin_file);

  TEST(init_null_file_fail);
	TEST(init_file_not_found_fail);
	TEST(init_simple_success);

  TEST(create_file_null_argument_failure);
  TEST(create_file_null_byte_filename_failure);
  TEST(create_file_truncate);
  TEST(create_file_duplicate_filename_failure);
  TEST(create_file_simple_success);
  TEST(create_file_skip_null_success);

  TEST(resize_file_does_not_exist_fail);
  TEST(resize_file_insufficient_space_fail); // NOT FULLY TESTED
  TEST(resize_file_decrease_size_success);
  TEST(resize_file_increase_size_success);

  TEST(repack_file_success);
  TEST(repack_file_insufficient_space_fail);

  TEST(delete_file_NULL_failure);
  TEST(delete_file_not_found_failure);
  TEST(delete_file_success);

  TEST(rename_file_not_found_fail);
  TEST(rename_file_duplicate_found_fail);
  TEST(rename_file_success);

  TEST(read_file_not_found_fail);
  TEST(read_file_overflow_fail);
  TEST(read_file_simple_success);

  TEST(write_file_not_found_fail);
  TEST(write_file_offset_greater_fail);
  TEST(write_file_insufficient_space_fail); // LIMITS NOT TESTED
  TEST(write_file_simple_success);
  TEST(write_file_repack_success);

  TEST(compute_hash_block_test);

  // TEST(read_bin_file);

  return 0;
}
