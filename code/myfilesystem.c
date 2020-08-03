#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <pthread.h>

// fread and fwrite need error checks?

// #define MAX_RECORDS (3) // for testing
#define MAX_RECORDS (2 ^ 16)
#define MAX_BLOCKS (2 ^ 24)
#define FILE_DATA_SIZE (256 * MAX_BLOCKS)
#define DIR_TABLE_SIZE (72 * MAX_RECORDS)

#include "myfilesystem.h"

/************************************************************************************************/
/* Structs */

// #pragma pack ( 1 ) // prevents struct from using padding // commented out for local machine
typedef struct file_details {
	char filename[64];
	unsigned offset;
	unsigned length;
} file_details;

typedef struct real_files {
	FILE * f1;
	FILE * f2;
	FILE * f3;
	int existing_records;
	int max_available_records;
	size_t f2_byte_size;
} real_files;

// FIX type of hash should not be char?
typedef struct hash_block {
	char hash[64];
} hash_block;

/************************************************************************************************/
/* Helper functions */

char * truncate_filename(char* filename) {
	if(strlen(filename) > 64) {
		char dest[64];
		memcpy(dest, filename, 64);
		dest[63] = '\0';
		filename = dest;
	}
	return filename;
}

size_t file_size_check(FILE * fp) {
	fseek(fp, 0L, SEEK_END);
	return ftell(fp);
}

int real_file_size_check(void * helper) {
	real_files* r_files = (real_files*) helper;
	size_t f1_size = file_size_check(r_files->f1);
	size_t f2_size = file_size_check(r_files->f2);
	if(f1_size > FILE_DATA_SIZE || f2_size > DIR_TABLE_SIZE) {
		perror("The real files have exceeded their allowed size.");
		printf("f1 size: %ld\nf2 size: %ld\n", f1_size, f2_size);
		return 1;
	}
	return 0;
}

void print_sample_file_contents(FILE* fp, char* filename) {
	fseek(fp, 0, SEEK_SET);
  printf("-------%s contents are:-------\n", filename);
	char ch;
  int counter = 0;
	while ((ch = fgetc(fp)) != EOF) {
    if(strcmp(filename, "test_file1") != 0) {
      printf("%c", ch);
    } else {
      if(ch == '\0') {
        if(counter == 0) {
          printf("###");
        }
        counter++;
      } else {
        if(counter > 0) {
          printf("%d###", counter);
          counter = 0;
        }
        printf("%c", ch);
      }
    }
  }
	printf("\n-------end of %s contents-------\n", filename);
}

file_details* fread_all_into_malloc_dir_table(FILE* fp, int f2_byte_size, int max_available_records) {
	file_details* records = (file_details*) malloc(f2_byte_size);
	fseek(fp, 0, SEEK_SET);
	fread(records, sizeof(file_details), max_available_records, fp);
	return records;
}

void fwrite_all_from_malloc_dir_table(FILE* fp, file_details* f2_malloc, int f2_byte_size) {
	fseek(fp, 0, SEEK_SET);
	fwrite(f2_malloc, f2_byte_size, 1, fp);
	fflush(fp);
}

int compare_file_details_struct(const void* a, const void* b) {
	file_details* x = (file_details*) a;
	file_details* y = (file_details*) b;
	if(x->filename[0] == '\0') {
		return 1; // x after y
	} else if(y->filename[0] == '\0') {
		return -1; // x before y
	} else if(x->offset > y->offset) {
		return 1; // x after y
	} else if(x->offset == y->offset) {
		return 0; // no change
	} else {
		return -1; // x before y
	}
}

int find_available_offset(int space_required, int max_available_records, file_details * sorted_records) {
	// find offset available
	int offset = -1;
	// if first record is empty means there are no records
	if(sorted_records[0].filename[0] == '\0') {
		offset = 0;
	// if can fit before first record
	} else if(space_required < sorted_records[0].offset + 1) {
			offset = 0;
	// if only one record
	} else if(sorted_records[1].filename[0] == '\0') {
			offset = sorted_records[0].offset + sorted_records[0].length;
	} else {
		// more than one record
		for(int i = 0; i < max_available_records - 1; i++) {
			if((sorted_records[i + 1].offset - sorted_records[i].offset - sorted_records[i].length) >= space_required) {
				offset = sorted_records[i].offset + sorted_records[i].length;
				break;
			}
		}
	}
	return offset;
}

int find_record_in_dir_table(char* filename, int max_available_records, file_details* records) {
	// find file pos in directory table
	// error if filename does not exist in directory table
	int file_pos = -1;
	for(int i = 0; i < max_available_records; i++) {
		if(strncmp(records[i].filename, truncate_filename(filename), 64) == 0) {
			file_pos = i;
			break;
		}
	}
	return file_pos;
}

int verify_hash_tree(void * helper);

/************************************************************************************************/

// mutex allocated in static
// mutex initialised with MACRO
pthread_mutex_t my_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t write_mutex = PTHREAD_MUTEX_INITIALIZER;

// assume files are empty
// not sure what n_processors are
void * init_fs(char * f1, char * f2, char * f3, int n_processors) {

	// initialise mutex with function
	pthread_mutex_init(&my_mutex, NULL);
	pthread_mutex_init(&write_mutex, NULL);

	real_files* r_files = (real_files*) malloc(sizeof(real_files));
	if(f1 != NULL && f2 != NULL && f3 != NULL) {
		if((r_files->f1 = fopen(f1, "rb+")) == NULL) {
			// perror("Unable to open real file 1");
			free(r_files);
			return NULL;
		}
		if((r_files->f2 = fopen(f2, "rb+")) == NULL) {
			// perror("Unable to open real file 2");
			free(r_files);
			return NULL;
		}
		if((r_files->f3 = fopen(f3, "rb+")) == NULL) {
			// perror("Unable to open real file 3");
			free(r_files);
			return NULL;
		}

		r_files->f2_byte_size = file_size_check(r_files->f2);
		r_files->max_available_records = r_files->f2_byte_size/72;
		r_files->existing_records = 0;
		file_details* record = (file_details*) malloc(sizeof(file_details));
	  for(int i = 0; i < r_files->max_available_records; i++) {
			fseek(r_files->f2, i * sizeof(file_details), SEEK_SET);
			fread(record, sizeof(*record), 1, r_files->f2);
			if(record->length >= 0 &&  strlen(record->filename) > 0) {
				r_files->existing_records++;
				// printf("record %d: %s, offset: %d, length: %d\n", i, record->filename, record->offset, record->length);
			}
		}
		free(record);
		fseek(r_files->f2, 0, SEEK_SET);
		// printf("Init_fs: There are %d records in directory table.\n", r_files->existing_records);
		return r_files;
	} else {
		// perror("One of the three real files are NULL.");
		free(r_files);
		return NULL;
	}
}

void close_fs(void * helper) {
	if(helper) { // only free if helper is not NULL
		real_files* r_files = (real_files*) helper;
		fclose(r_files->f1);
		fclose(r_files->f2);
		fclose(r_files->f3);
		free(r_files);
		pthread_mutex_destroy(&my_mutex);
	}
}

int create_file(char * filename, size_t length, void * helper) {

		pthread_mutex_lock(&my_mutex);

		if(filename == NULL || !length || helper == NULL || strlen(filename) <= 0) {
			// perror("Argument Error: failed to create file");
			pthread_mutex_unlock(&my_mutex);
			return 1;
		}
		if(real_file_size_check(helper)) {
			pthread_mutex_unlock(&my_mutex);
			return 1;
		}

		real_files* r_files = (real_files*) helper;

		if(r_files->existing_records + 1 > r_files->max_available_records) {
				// perror("There is no space for more files.");
				pthread_mutex_unlock(&my_mutex);
				return 1;
		}

		// malloc for sorted directory table
		file_details* sorted_records = fread_all_into_malloc_dir_table(r_files->f2, r_files->f2_byte_size, r_files->max_available_records);
		qsort(sorted_records, r_files->max_available_records, sizeof(file_details), compare_file_details_struct);

		// finds file with the smallest available file (at least length bytes of contiguous space)
		int determined_offset = find_available_offset(length, r_files->max_available_records, sorted_records);

		// if f1 is not big enough to fit the virtual file
		if(determined_offset + length > file_size_check(r_files->f1)) {
		  // perror("File Data was not big enough to fit the virtual file.");
			repack(helper);

			// update records after repack using realloc
			sorted_records = (file_details*) realloc(sorted_records, r_files->f2_byte_size);
			fseek(r_files->f2, 0, SEEK_SET);
			fread(sorted_records, sizeof(file_details), r_files->max_available_records, r_files->f2);
			qsort(sorted_records, r_files->max_available_records, sizeof(file_details), compare_file_details_struct);

			// finds the next available slot
			determined_offset = find_available_offset(length, r_files->max_available_records, sorted_records);
			if(determined_offset < 0 || (determined_offset + length > file_size_check(r_files->f1))) {
				// perror("There is insufficient space overall, even after repacking.");
				free(sorted_records);
				pthread_mutex_unlock(&my_mutex);
				return 2;
			}
		}
		free(sorted_records);

		//////// THERE IS SPACE TO WRITE TO FILE ////////

		// malloc for unsorted directory table
		file_details* records = fread_all_into_malloc_dir_table(r_files->f2, r_files->f2_byte_size, r_files->max_available_records);

		// check if file exists in directory malloc
		int file_pos = find_record_in_dir_table(filename, r_files->max_available_records, records);
		if(file_pos >= 0) {
			// perror("File exists in directory table");
			free(records);
			pthread_mutex_unlock(&my_mutex);
			return 1;
		}

		// write data into available space
		for(int i = 0; i < r_files->max_available_records; i++) {
			if(records[i].filename[0] == '\0') { // add metadata to file_details struct
				strncpy(records[i].filename, truncate_filename(filename), 64);
				records[i].length = length;
				records[i].offset = determined_offset;
				// printf("%d: %s length %d offset %d\n", i, records[i].filename, records[i].length, records[i].offset);
				break;
			}
		}

		// // COMMENT OUT FOR EDSTEM
		// for(int i = 0; i < r_files->max_available_records; i++) {
		// 	printf("%d: %s length %d offset %d\n", i, records[i].filename, records[i].length, records[i].offset);
		// }
		// print_sample_file_contents(r_files->f1, "test_file1");

		// write directly to directory table
		fwrite_all_from_malloc_dir_table(r_files->f2, records, r_files->f2_byte_size);
		r_files->existing_records++;

		// write directly to file data
		char buffer[length];
		memset(buffer, '\0', length);
		fseek(r_files->f1, determined_offset, SEEK_SET);
		fwrite(buffer, sizeof(buffer), 1, r_files->f1);
		fflush(r_files->f1);

		// FIX update hash data, may be much slower than compute_hash_block
		compute_hash_tree(helper);

		free(records);
		pthread_mutex_unlock(&my_mutex);
		return 0;
}

int resize_file(char * filename, size_t length, void * helper) {

		pthread_mutex_lock(&my_mutex);

		if(filename == NULL || !length || helper == NULL || strlen(filename) <= 0) {
			// perror("Argument Error: failed to create file");
			pthread_mutex_unlock(&my_mutex);
			return 1;
		}
		real_files* r_files = (real_files*) helper;

		if(r_files->existing_records + 1 > r_files->max_available_records) {
				// perror("There is no space for more files.");
				pthread_mutex_unlock(&my_mutex);
				return 1;
		}

		// malloc for sorted directory table
		file_details* sorted_records = fread_all_into_malloc_dir_table(r_files->f2, r_files->f2_byte_size, r_files->max_available_records);
		// original pos in unsorted dir table
		int file_og_pos = find_record_in_dir_table(filename, r_files->max_available_records, sorted_records);
		qsort(sorted_records, r_files->max_available_records, sizeof(file_details), compare_file_details_struct);

		int file_pos = find_record_in_dir_table(filename, r_files->max_available_records, sorted_records);
		if(file_pos < 0) {
			// perror("Cannot find File in directory table");
			free(sorted_records);
			pthread_mutex_unlock(&my_mutex);
			return 1;
		}

		//////// REPACK IF NEW SIZE CANNOT FIT INTO CURRENT SPACE //////////

		// if record is at the end of the list or its next record is NULL and
		// if new length exceeds the file byte size
		int last_record_in_list = file_pos >= r_files->max_available_records - 1 || sorted_records[file_pos + 1].filename[0] == '\0';
		int length_exceed_file_size = sorted_records[file_pos].offset + length > file_size_check(r_files->f1);

		// if next record[file_pos + 1] is a Record and new length exceeds next offset
		int next_is_record = sorted_records[file_pos + 1].filename[0] != '\0';
		int new_length_exceeds_next_offset = sorted_records[file_pos].offset + length > sorted_records[file_pos + 1].offset;

		if((last_record_in_list && length_exceed_file_size) || (next_is_record && new_length_exceeds_next_offset)) {
			// printf("new length cant fit, have to repack\n");
			repack(helper);

			// update records after repack using realloc
			sorted_records = (file_details*) realloc(sorted_records, r_files->f2_byte_size);
			fseek(r_files->f2, 0, SEEK_SET);
			fread(sorted_records, sizeof(file_details), r_files->max_available_records, r_files->f2);
			qsort(sorted_records, r_files->max_available_records, sizeof(file_details), compare_file_details_struct);

			// check if there is space to fit a record
			int determined_offset = find_available_offset(length, r_files->max_available_records, sorted_records);
			// if does not exceed file size (minus file pos length from the calculation since going to resize)
			if(determined_offset < 0 || (determined_offset + length - sorted_records[file_pos].length > file_size_check(r_files->f1))) {
				// perror("There is insufficient space overall, even after repacking.");
				free(sorted_records);
				pthread_mutex_unlock(&my_mutex);
				return 2;
			}

			// new position after repack and sort
			int new_file_pos = find_record_in_dir_table(filename, r_files->max_available_records, sorted_records);


			////////// COPY FILE DATA TO NEW POSITION  //////////

			// if file is the not the last record and new file pos cannot accomodate the new size
			last_record_in_list = new_file_pos >= r_files->max_available_records - 1 || sorted_records[new_file_pos + 1].filename[0] == '\0';
		  if(!last_record_in_list && new_file_pos != determined_offset) {
				file_pos = new_file_pos;
				char buffer2[sorted_records[file_pos].length];
				fseek(r_files->f1, sorted_records[file_pos].offset, SEEK_SET);
				fread(buffer2, sizeof(buffer2), 1, r_files->f1);
				fseek(r_files->f1, determined_offset, SEEK_SET);
				fwrite(buffer2, sizeof(buffer2), 1, r_files->f1);
				fflush(r_files->f1);
				// save the offset which has enough space for new size
				sorted_records[file_pos].offset = determined_offset;

				// FIX update hash data, may be much slower than compute_hash_block
				compute_hash_tree(helper);
		  }
		}

		// write directly to f1 (FILE DATA)
		// if increasing size of file, fill new space with zeros
		if(length > sorted_records[file_pos].length) {
			int length_diff = length - sorted_records[file_pos].length;
			char buffer[length_diff];
			memset(buffer, '\0', length_diff);
			fseek(r_files->f1, sorted_records[file_pos].offset + sorted_records[file_pos].length, SEEK_SET);
			fwrite(buffer, sizeof(buffer), 1, r_files->f1);
			fflush(r_files->f1);

			// FIX update hash data, may be much slower than compute_hash_block
			compute_hash_tree(helper);
		}
		// decreasing size of file, file length will be truncated anyway
		sorted_records[file_pos].length = length;

		// write new offset and length to directory table without using sorted
		fseek(r_files->f2, file_og_pos * sizeof(file_details), SEEK_SET);
		fwrite(&sorted_records[file_pos], sizeof(file_details), 1, r_files->f2);
		fflush(r_files->f2);

		// repack if first record is 0
		file_details* records = fread_all_into_malloc_dir_table(r_files->f2, r_files->f2_byte_size, r_files->max_available_records);
		qsort(records, r_files->max_available_records, sizeof(file_details), compare_file_details_struct);
		if(records[0].offset != 0) {
			repack(helper);
		}
		free(records);

		free(sorted_records);
		pthread_mutex_unlock(&my_mutex);
	  return 0;
};

void repack(void * helper) {
		// if(helper == NULL) {
		// 	perror("Argument Error: failed to create file");
		// }
		real_files* r_files = (real_files*) helper;

		// malloc for sorted directory table
		file_details* records = fread_all_into_malloc_dir_table(r_files->f2, r_files->f2_byte_size, r_files->max_available_records);
		file_details* sorted_records = fread_all_into_malloc_dir_table(r_files->f2, r_files->f2_byte_size, r_files->max_available_records);
		qsort(sorted_records, r_files->max_available_records, sizeof(file_details), compare_file_details_struct);

		// shift offsets of files to the left
		char* file_data = (char*) malloc(sizeof(r_files->f1)); // only if using memcpy
		int curr_offset = 0;
		// only iterate based on the number of records found
		for(int i = 0; i < r_files->existing_records; i++) {
			if(sorted_records[i].offset > curr_offset) {
				// printf("change %s offset from %d to %d\n", sorted_records[i].filename, sorted_records[i].offset, curr_offset);
				char buffer[sorted_records[i].length];
				fseek(r_files->f1, sorted_records[i].offset, SEEK_SET);
				fread(buffer, sizeof(buffer), 1, r_files->f1);
				fseek(r_files->f1, curr_offset, SEEK_SET);
				fwrite(buffer, sizeof(buffer), 1, r_files->f1);
				fflush(r_files->f1);

				// FIX update hash data, may be much slower than compute_hash_block
				compute_hash_tree(helper);
			}
			// copy updated offsets to unsorted directory table
			for(int j = 0; j < r_files->max_available_records; j++) {
				if(records[j].filename[j] != '\0' && strncmp(records[j].filename, sorted_records[i].filename, 64) == 0) {
					records[j].offset = curr_offset;
					break;
				}
			}
			// add new offset to sorted table, increment next available offset
			sorted_records[i].offset = curr_offset;
			curr_offset += sorted_records[i].length;
		}
		free(file_data);

		// write the updated offsets to directory_table
		fwrite_all_from_malloc_dir_table(r_files->f2, records, r_files->f2_byte_size);

		free(records);
		free(sorted_records);
		return;
}

int delete_file(char * filename, void * helper) {

	pthread_mutex_lock(&my_mutex);

	if(filename == NULL || helper == NULL || strlen(filename) <= 0) {
		// perror("Argument Error: failed to create file");
		pthread_mutex_unlock(&my_mutex);
		return 1;
	}

	real_files* r_files = (real_files*) helper;

	// truncate filename
	filename = truncate_filename(filename);

	// create a malloced sized file
	file_details* records = fread_all_into_malloc_dir_table(r_files->f2, r_files->f2_byte_size, r_files->max_available_records);

	int file_pos = find_record_in_dir_table(filename, r_files->max_available_records, records);
	if(file_pos < 0) {
		// perror("Cannot find File in directory table");
		free(records);
		pthread_mutex_unlock(&my_mutex);
		return 1;
	}

	// remove data from record
	memset(records[file_pos].filename, '\0', 64);
	records[file_pos].offset = 0;
	records[file_pos].length = 0;

	// write directly to directory table
	fwrite_all_from_malloc_dir_table(r_files->f2, records, r_files->f2_byte_size);
	r_files->existing_records--;

	free(records);
	pthread_mutex_unlock(&my_mutex);
  return 0;
}

int rename_file(char * oldname, char * newname, void * helper) {

	pthread_mutex_lock(&my_mutex);

	if(oldname == NULL || newname == NULL || helper == NULL || strlen(oldname) <= 0 || strlen(newname) <= 0) {
		// perror("Argument Error: failed to create file");
		pthread_mutex_unlock(&my_mutex);
		return 1;
	}
	real_files* r_files = (real_files*) helper;

	// malloc for directory table
	file_details* records = fread_all_into_malloc_dir_table(r_files->f2, r_files->f2_byte_size, r_files->max_available_records);

	// find file pos in directory table
	// error if filename does not exist in directory table
	int file_pos = -1;
	for(int i = 0; i < r_files->max_available_records; i++) {
		// find a virtual file with name == oldname
		if(file_pos < 0 && strncmp(records[i].filename, truncate_filename(oldname), 64) == 0) {
			file_pos = i;
		}
		// find duplicate virtual files with name newname
		if(strncmp(records[i].filename, truncate_filename(newname), 64) == 0) {
			file_pos = -1;
			break;
		}
	}
	if(file_pos < 0) {
		// perror("Failed to rename file: oldname not found or newname exists");
		free(records);
		pthread_mutex_unlock(&my_mutex);
		return 1;
	}

	// write directly to directory table
	strncpy(records[file_pos].filename, truncate_filename(newname), 64);
	fwrite_all_from_malloc_dir_table(r_files->f2, records, r_files->f2_byte_size);

	free(records);
	pthread_mutex_unlock(&my_mutex);
  return 0;
}

int read_file(char * filename, size_t offset, size_t count, void * buf, void * helper) {

	pthread_mutex_lock(&my_mutex);

	if(filename == NULL || offset < 0 || !count || buf == NULL ||
			helper == NULL || strlen(filename) <= 0) {
		// perror("Argument Error: failed to create file");
		pthread_mutex_unlock(&my_mutex);
		return 1;
	}
	real_files* r_files = (real_files*) helper;


	// FIX update hash data, may be much slower than compute_hash_block
	if(verify_hash_tree(helper)) {
		pthread_mutex_unlock(&my_mutex);
		return 3; // verification failed
	}

	// malloc for directory table
	file_details* records = fread_all_into_malloc_dir_table(r_files->f2, r_files->f2_byte_size, r_files->max_available_records);

	int file_pos = find_record_in_dir_table(filename, r_files->max_available_records, records);
	if(file_pos < 0) {
		// perror("Cannot find File in directory table");
		pthread_mutex_unlock(&my_mutex);
		free(records);
		return 1;
	}
	if(records[file_pos].length < count || records[file_pos].length < offset || r_files->f2_byte_size < records[file_pos].offset + count) {
		// perror("Cannot read bytes from file, overflow detected");
		pthread_mutex_unlock(&my_mutex);
		free(records);
		return 2;
	}

	// store data into buf variable
	char buffer[count + 1];
	fseek(r_files->f1, records[file_pos].offset + offset, SEEK_SET);
	fread(buffer, count, 1, r_files->f1);
	buffer[count] = '\0';
	memcpy(buf, buffer, sizeof(buffer));

	free(records);
	pthread_mutex_unlock(&my_mutex);
	return 0;
}

int write_file(char * filename, size_t offset, size_t count, void * buf, void * helper) {
	if(filename == NULL || offset < 0 || !count || buf == NULL ||
			helper == NULL || strlen(filename) <= 0) {
		// perror("Argument Error: failed to create file");
		return 1;
	}
	real_files* r_files = (real_files*) helper;

	// malloc for sorted directory table
	file_details* sorted_records = fread_all_into_malloc_dir_table(r_files->f2, r_files->f2_byte_size, r_files->max_available_records);
	// original pos in unsorted dir table
	int file_og_pos = find_record_in_dir_table(filename, r_files->max_available_records, sorted_records);
	qsort(sorted_records, r_files->max_available_records, sizeof(file_details), compare_file_details_struct);

	int file_pos = find_record_in_dir_table(filename, r_files->max_available_records, sorted_records);
	if(file_pos < 0) {
		// perror("Cannot find File in directory table");
		free(sorted_records);
		return 1;
	}
	if(sorted_records[file_pos].length < offset) {
		// perror("Invalid offset, overflow detected");
		free(sorted_records);
		return 2;
	}

	// if current filesize is smaller than count & offset
	if(sorted_records[file_pos].length < count || sorted_records[file_pos].length < count + offset) {
		int resz_val = resize_file(sorted_records[file_pos].filename, count + offset, helper);

		// record exist check covered before, dont need resz_val == 1
		if(resz_val == 2) {
			// perror("Insufficient Space in disk for new file size");
			free(sorted_records);
			return 3;
		} else if(resz_val != 0) {
			// perror("Uncaught error detected when resizing file");
			free(sorted_records);
			return 4;
		}

		// update records after repack using realloc
		sorted_records = (file_details*) realloc(sorted_records, r_files->f2_byte_size);
		fseek(r_files->f2, 0, SEEK_SET);
		fread(sorted_records, sizeof(file_details), r_files->max_available_records, r_files->f2);
		qsort(sorted_records, r_files->max_available_records, sizeof(file_details), compare_file_details_struct);

		// new position after resize
		file_pos = find_record_in_dir_table(filename, r_files->max_available_records, sorted_records);
	}

	pthread_mutex_lock(&write_mutex);
	// find file pos and write buf data into f1 (file_data)
	fseek(r_files->f1, sorted_records[file_pos].offset + offset, SEEK_SET);
	fwrite(buf, count, 1, r_files->f1);
	fflush(r_files->f1);

	// write new length to directory table without using sorted
	fseek(r_files->f2, file_og_pos * sizeof(file_details), SEEK_SET);
	fwrite(&sorted_records[file_pos], sizeof(file_details), 1, r_files->f2);
	fflush(r_files->f2);
	pthread_mutex_unlock(&write_mutex);
	// FIX update hash data, may be much slower than compute_hash_block
	compute_hash_tree(helper);

	free(sorted_records);

	return 0;
}

ssize_t file_size(char * filename, void * helper) {

	pthread_mutex_lock(&my_mutex);

	if(filename == NULL || helper == NULL || strlen(filename) <= 0) {
		// perror("Argument Error: failed to create file");
		pthread_mutex_unlock(&my_mutex);
		return 1;
	}
	real_files* r_files = (real_files*) helper;

	// malloc for directory table
	file_details* records = (file_details*) malloc(r_files->f2_byte_size);
	fseek(r_files->f2, 0, SEEK_SET);
	fread(records, sizeof(file_details), r_files->max_available_records, r_files->f2);

	// find file pos in directory table and return its length
	int file_pos = find_record_in_dir_table(filename, r_files->max_available_records, records);
	if(file_pos >= 0) {
		int file_size = records[file_pos].length;
		free(records);
		pthread_mutex_unlock(&my_mutex);
		return file_size;
	}

	// perror("Cannot find File in directory table");
	free(records);
	pthread_mutex_unlock(&my_mutex);
  return -1;
}

// reading byte by byte
void fletcher(uint8_t * buf, size_t length, uint8_t * output) {
		if(buf == NULL || length < 0 || output == NULL ) {
			perror("Argument Error: failed fletcher");
			return;
		}

		uint32_t* concat_bytes = (uint32_t*) malloc(sizeof(uint32_t));
		int num_of_4_bytes = length / 4; // rounded
		uint64_t fletcher_hash_pow = (uint64_t) pow(2, 32) - 1;
		uint64_t a = 0;
		uint64_t b = 0;
		uint64_t c = 0;
		uint64_t d = 0;

		int count = 0;
		for(int i = 0; i < length; i+=4) {
			memset(concat_bytes, 0, 4); // reset to 0, plus solves for padding

			// if not multiple of 4 and if it is remainder bytes
			if(length % 4 != 0 && count == num_of_4_bytes) {
				for(int j = 0; j < length % 4; j++) { // for padding
					memcpy(concat_bytes, &buf[i], length - (4 * num_of_4_bytes));
					memcpy(concat_bytes, &buf[i], length % 4);
				}
			} else {
				for(int j = 0; j < 4; j++) { // add 4 ints to the array
					memcpy(concat_bytes, &buf[i], 4);
				}
			}

			// add concat bytes to variables
			a = (a + *concat_bytes) % fletcher_hash_pow;
			b = (b + a) % fletcher_hash_pow;
			c = (c + b) % fletcher_hash_pow;
			d = (d + c) % fletcher_hash_pow;

			count++;
		}

		uint32_t temp_arr[4] = {
			(uint32_t) a,
			(uint32_t) b,
			(uint32_t) c,
			(uint32_t) d
		};
		int counter = 0;
		// concatenate significant bytes of each 32 bit variable into output
		for(int k = 0; k < 16; k+=4) {
			memcpy(&output[k], &temp_arr[counter], 4);
			counter++;
		}
		free(concat_bytes);
    return;
}

/************************************************************************************************/

void update_hash_block(size_t hash_offset, void * helper) {
	if(hash_offset < 0 || helper == NULL) {
		// perror("Argument Error: update hash block");
		return;
	}

	real_files* r_files = (real_files*) helper;

	int log_num_blocks_f1 = log2(file_size_check(r_files->f1)/256);
	int num_levels_f3 = log_num_blocks_f1 + 1;
	int leaves_start_index = pow(2, num_levels_f3 - 1) - 1;
	// int num_hash_blocks_f3 = pow(2, num_levels_f3) - 1;

	if(hash_offset >= leaves_start_index) { // hash offset is a leaf
		// printf("hash is a leaf, reading from %ld and writing to %ld\n", hash_offset - leaves_start_index, hash_offset);

		// read block data from BLOCK offset
		uint8_t* block_data = (uint8_t*) malloc(256);
		fseek(r_files->f1, (hash_offset - leaves_start_index) * 256, SEEK_SET);
		fread(block_data, 256, 1, r_files->f1);

		// convert data into hash block
		uint8_t* output_hash = (uint8_t*) malloc(16);
		fletcher(block_data, 256, output_hash);

		// write hash value into f3 at HASH offset
		fseek(r_files->f3, hash_offset * 16, SEEK_SET);
		fwrite(output_hash, 16, 1, r_files->f3);
		fflush(r_files->f3);

		free(output_hash);
		free(block_data);
	} else { // block offset is a parent
		// position of left and right children
		int left_index = (2 * hash_offset) + 1;
		int right_index = (2 * hash_offset) + 2;
		// printf("merging %ld with left %d with right %d\n", hash_offset, left_index, right_index);

		// read hash of left and right children
		uint8_t* left_hash = (uint8_t*) malloc(16);
		uint8_t* right_hash = (uint8_t*) malloc(16);
		fseek(r_files->f3, left_index * 16, SEEK_SET);
		fread(left_hash, 16, 1, r_files->f3);
		fseek(r_files->f3, right_index * 16, SEEK_SET);
		fread(right_hash, 16, 1, r_files->f3);

		// concatenate hash of left and right
		uint8_t* concat_hash = (uint8_t*) malloc(32);
		memcpy(concat_hash, left_hash, 16);
		memcpy(concat_hash + 16, right_hash, 16);

		// re-hash the concatenated hash
		uint8_t* output_hash = (uint8_t*) malloc(16);
		fletcher(concat_hash, 32, output_hash);

		// write final hashed value to f3 at HASH offset
		fseek(r_files->f3, hash_offset * 16, SEEK_SET);
		fwrite(output_hash, 16, 1, r_files->f3);
		fflush(r_files->f3);

		free(output_hash);
		free(concat_hash);
		free(left_hash);
		free(right_hash);
	}
	return;
}

void recursive_hash(int index, int max_level, void * helper) {
	if(index < 0 || max_level < 0 || helper == NULL) {
		// perror("Argument Error: failed perform recursive hash");
		return;
	}

	// printf("recursive_hash args: index %d, max level %d\n", index, max_level);

	// find HASH offset cur level
	int cur_level = max_level;
	while(index < pow(2, cur_level - 1) - 1)
		cur_level--;

	// base case, if hash is a leaf
	if(cur_level == max_level) {
		update_hash_block(index, helper);
		return;
	}
	// recursive, if hash has children
	// left child and right child HASH offsets are 2k + 1, 2k + 2 respectively
	recursive_hash((2 * index) + 1, max_level, helper); // left child
	recursive_hash((2 * index) + 2, max_level, helper); // right child
	update_hash_block(index, helper); // re-hash the parent
	return;
}

int verify_hash_block(size_t hash_offset, void * helper) {
	if(hash_offset < 0 || helper == NULL) {
		perror("Argument Error: verify hash block");
		return 1;
	}

	real_files* r_files = (real_files*) helper;

	int log_num_blocks_f1 = log2(file_size_check(r_files->f1)/256);
	int num_levels_f3 = log_num_blocks_f1 + 1;
	int leaves_start_index = pow(2, num_levels_f3 - 1) - 1;
	// int num_hash_blocks_f3 = pow(2, num_levels_f3) - 1;

	if(hash_offset >= leaves_start_index) { // hash offset is a leaf
		// printf("hash is a leaf, reading from %ld and writing to %ld\n", hash_offset - leaves_start_index, hash_offset);

		// read block data from BLOCK offset
		uint8_t* block_data = (uint8_t*) malloc(256);
		fseek(r_files->f1, (hash_offset - leaves_start_index) * 256, SEEK_SET);
		fread(block_data, 256, 1, r_files->f1);

		// convert data into hash block
		uint8_t* output_hash = (uint8_t*) malloc(16);
		fletcher(block_data, 256, output_hash);

		// read hash value from f3 at HASH offset
		uint8_t* correct_hash = (uint8_t*) malloc(16);
		fseek(r_files->f3, hash_offset * 16, SEEK_SET);
		fread(correct_hash, 16, 1, r_files->f3);

		// verify hashes by comparing
		if(memcmp(correct_hash, output_hash, 16) != 0) {
			free(correct_hash);
			free(output_hash);
			free(block_data);
			return 1; // return error
		}

		free(correct_hash);
		free(output_hash);
		free(block_data);
	} else { // block offset is a parent
		// position of left and right children
		int left_index = (2 * hash_offset) + 1;
		int right_index = (2 * hash_offset) + 2;
		// printf("merging %ld with left %d with right %d\n", hash_offset, left_index, right_index);

		// read hash of left and right children
		uint8_t* left_hash = (uint8_t*) malloc(16);
		uint8_t* right_hash = (uint8_t*) malloc(16);
		fseek(r_files->f3, left_index * 16, SEEK_SET);
		fread(left_hash, 16, 1, r_files->f3);
		fseek(r_files->f3, right_index * 16, SEEK_SET);
		fread(right_hash, 16, 1, r_files->f3);

		// concatenate hash of left and right
		uint8_t* concat_hash = (uint8_t*) malloc(32);
		memcpy(concat_hash, left_hash, 16);
		memcpy(concat_hash + 16, right_hash, 16);

		// re-hash the concatenated hash
		uint8_t* output_hash = (uint8_t*) malloc(16);
		fletcher(concat_hash, 32, output_hash);

		// read final hashed value to f3 at HASH offset
		uint8_t* correct_hash = (uint8_t*) malloc(16);
		fseek(r_files->f3, hash_offset * 16, SEEK_SET);
		fread(correct_hash, 16, 1, r_files->f3);

		// verify hashes by comparing
		if(memcmp(correct_hash, output_hash, 16) != 0) {
			free(correct_hash);
			free(output_hash);
			free(concat_hash);
			free(left_hash);
			free(right_hash);
			return 1; // return error
		}

		free(correct_hash);
		free(output_hash);
		free(concat_hash);
		free(left_hash);
		free(right_hash);
	}
	return 0;
}

int verify_recursive_hash(int index, int max_level, void * helper) {
	if(index < 0 || max_level < 0 || helper == NULL) {
		perror("Argument Error: verify_hash_recursive");
		return -1;
	}

	// find HASH offset cur level
	int cur_level = max_level;
	while(index < pow(2, cur_level - 1) - 1)
		cur_level--;

	// base case, if hash is a leaf
	if(cur_level == max_level) {
		return verify_hash_block(index, helper);
	}
	// recursive, if hash has children
	// left child and right child HASH offsets are 2k + 1, 2k + 2 respectively
	if(verify_recursive_hash((2 * index) + 1, max_level, helper)) // left child
		return 1; // return error
	if(verify_recursive_hash((2 * index) + 2, max_level, helper)) // right child
		return 1; // return error
	return verify_hash_block(index, helper); // verify the parent
}

int verify_hash_tree(void * helper) {
	if(helper == NULL) {
		perror("Argument Error: failed compute hash tree");
		return 1;
	}
	real_files* r_files = (real_files*) helper;

	int log_num_blocks_f1 = log2(file_size_check(r_files->f1)/256);
	int num_levels_f3 = log_num_blocks_f1 + 1;

	// start recursion of merkle tree from root
	return verify_recursive_hash(0, num_levels_f3, helper);
}

/************************************************************************************************/

void compute_hash_tree(void * helper) {
	if(helper == NULL) {
		perror("Argument Error: failed compute hash tree");
		return;
	}
	real_files* r_files = (real_files*) helper;

	int log_num_blocks_f1 = log2(file_size_check(r_files->f1)/256);
	int num_levels_f3 = log_num_blocks_f1 + 1;
	// int num_hash_blocks_f3 = (int) pow(2, num_levels_f3) - 1;

	// start recursion of merkle tree from root
	recursive_hash(0, num_levels_f3, helper);

  return;
}

void compute_hash_block(size_t block_offset, void * helper) {
	int max_blocks = pow(2, 24);
	if(block_offset < 0 || block_offset > max_blocks || helper == NULL) {
		// perror("Argument Error: failed compute hash block");
		return;
	}
	real_files* r_files = (real_files*) helper;

	int log_num_blocks_f1 = log2(file_size_check(r_files->f1)/256);
	int num_levels_f3 = log_num_blocks_f1 + 1;
	int leaves_start_index = pow(2, num_levels_f3 - 1) - 1;

	// re-hash HASH offset and write changes to f3
	update_hash_block(block_offset + leaves_start_index, helper);

	// start recursion of merkle tree from root
	recursive_hash(0, num_levels_f3, helper);

	return;
}
