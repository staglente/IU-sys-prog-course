#include "userfs.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>


enum {
	BLOCK_SIZE = 512,
	MAX_FILE_SIZE = 1024 * 1024 * 100,
};

/** Global error code. Set from any function on any error. */
static enum ufs_error_code ufs_error_code = UFS_ERR_NO_ERR;

struct block {
	/** Block memory. */
	char *memory;
	/** How many bytes are occupied. */
	int occupied;
	/** Next block in the file. */
	struct block *next;
	/** Previous block in the file. */
	struct block *prev;

	/* PUT HERE OTHER MEMBERS */
	int overwritten;
};

struct file {
	/** Double-linked list of file blocks. */
	struct block *block_list;
	/**
	 * Last block in the list above for fast access to the end
	 * of file.
	 */
	struct block *last_block;
	/** How many file descriptors are opened on the file. */
	int refs;
	/** File name. */
	char *name;
	/** Files are stored in a double-linked list. */
	struct file *next;
	struct file *prev;


	/* PUT HERE OTHER MEMBERS */
	bool ghost;
	int blocks_count;
	int file_size;
};

/** List of all files. */
static struct file *file_list = NULL;

struct filedesc {
	struct file *file;

	/* PUT HERE OTHER MEMBERS */
	int rw_ptr;
	int flag;
};

/**
 * An array of file descriptors. When a file descriptor is
 * created, its pointer drops here. When a file descriptor is
 * closed, its place in this array is set to NULL and can be
 * taken by next ufs_open() call.
 */
static struct filedesc **file_descriptors = NULL;
static int file_descriptor_count = 0;
static int file_descriptor_capacity = 0;

enum ufs_error_code
ufs_errno()
{
	return ufs_error_code;
}

int
ufs_open(const char *filename, int flags)
{
    // Check if the filename is valid
    if (filename == NULL || strlen(filename) == 0) {
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    }
    // Check if the file already exists
    struct file *file_ptr = file_list;
		bool ch_filename = false;
    while (file_ptr != NULL) {
        if (strcmp(file_ptr->name, filename) == 0) {
            // File already exists
						if (file_ptr->ghost == true) {
							if ((flags != 0)){
								break;
							}
							ufs_error_code = UFS_ERR_NO_FILE;
			        return -1;
						}
            file_ptr->refs++;
						if (file_descriptor_count >= file_descriptor_capacity) {
				        // Increase the capacity of the file descriptor array
				        int new_capacity = file_descriptor_capacity + 10000;
				        struct filedesc **new_array = realloc(file_descriptors, new_capacity * sizeof(struct filedesc *));
				        if (new_array == NULL) {
				            ufs_error_code = UFS_ERR_NO_MEM;
				            return -1;
				        }
				        file_descriptors = new_array;
				        file_descriptor_capacity = new_capacity;
				    }

				    // Find an empty slot in the file descriptor array
				    int fd;
				    for (fd = 0; fd < file_descriptor_capacity; fd++) {
				        if (file_descriptors[fd] == NULL) {
				            struct filedesc *new_filedesc = malloc(sizeof(struct filedesc));
				            if (new_filedesc == NULL) {
				                ufs_error_code = UFS_ERR_NO_MEM;
				                return -1;
				            }
				            new_filedesc->file = file_ptr;
										new_filedesc->rw_ptr = 0;
										new_filedesc->flag = flags;
										struct block *iter = file_ptr->block_list;


										while(iter != NULL){
											iter->overwritten = 0;
											iter = iter->next;
										}
				            file_descriptors[fd] = new_filedesc;
				            file_descriptor_count++;
				            return fd + 1;  // Return the file descriptor index (1-based)
				        }

				    }

        }
        file_ptr = file_ptr->next;
    }

    // If UFS_CREATE flag is not set, return error
		if (!(flags & UFS_CREATE)) {
				ufs_error_code = UFS_ERR_NO_FILE;
				return -1;
		}


    // Create a new file
    struct file *new_file = malloc(sizeof(struct file));
    if (new_file == NULL) {
        ufs_error_code = UFS_ERR_NO_MEM;
        return -1;
    }
    // Set the file properties
    new_file->block_list = NULL;
    new_file->last_block = NULL;
    new_file->refs = 1;
    new_file->name = strdup(filename);
    new_file->next = NULL;
    new_file->prev = NULL;
		new_file->ghost = false;
		new_file->blocks_count = 0;
		new_file->file_size = 0;
    // Add the new file to the file list
    if (file_list == NULL) {
        // First file in the list
        file_list = new_file;
    } else {
        // Append to the end of the file list
        struct file *last_file = file_list;
        while (last_file->next != NULL) {
            last_file = last_file->next;
        }
        last_file->next = new_file;
        new_file->prev = last_file;
    }

    // Update the file descriptor array
    if (file_descriptor_count >= file_descriptor_capacity) {
        // Increase the capacity of the file descriptor array
        int new_capacity = file_descriptor_capacity + 10000;
        file_descriptors = realloc(file_descriptors, new_capacity * sizeof(struct filedesc *));
        if (file_descriptors == NULL) {
            ufs_error_code = UFS_ERR_NO_MEM;
            return -1;
        }
        file_descriptor_capacity = new_capacity;
    }

    // Find an empty slot in the file descriptor array
    int fd;
    for (fd = 0; fd < file_descriptor_capacity; fd++) {
        if (file_descriptors[fd] == NULL) {
            struct filedesc *new_filedesc = malloc(sizeof(struct filedesc));
            if (new_filedesc == NULL) {
                ufs_error_code = UFS_ERR_NO_MEM;
                return -1;
            }
            new_filedesc->file = new_file;
						new_filedesc->rw_ptr = 0;
						new_filedesc->flag = flags;
            file_descriptors[fd] = new_filedesc;
            file_descriptor_count++;
            return fd + 1;  // Return the file descriptor index (1-based)
        }
    }
    // No empty slot found in the file descriptor array
    ufs_error_code = UFS_ERR_NO_MEM;
    return -1;
}


ssize_t
ufs_write(int fd, const char *buf, size_t size) {
    if (fd <= 0 || file_descriptors[fd - 1] == NULL) {
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    }
		if(file_descriptors[fd - 1]->flag == UFS_READ_ONLY){
			ufs_error_code = UFS_ERR_NO_PERMISSION;
			return -1;
		}
		if (file_descriptors[fd - 1]->rw_ptr >= MAX_FILE_SIZE){
			ufs_error_code = UFS_ERR_NO_MEM;
			return -1;
		}

    struct file *file = file_descriptors[fd - 1]->file;
    if (file->refs <= 0) {
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    }

    struct block *current_block = file->block_list;
    size_t bytes_written = 0;
		int rw_ptr = file_descriptors[fd - 1]->rw_ptr;
		if (rw_ptr >= file->file_size) {
			rw_ptr = file->file_size;
			file_descriptors[fd - 1]->rw_ptr = file->file_size;
		}
		while(rw_ptr>=BLOCK_SIZE && current_block != NULL){
 			current_block->overwritten = BLOCK_SIZE;
			current_block->occupied = BLOCK_SIZE;
			rw_ptr -= BLOCK_SIZE;
			current_block = current_block->next;
		}
		if (current_block != NULL){
			current_block->overwritten = rw_ptr;
			struct block *next_blocks = current_block->next;
			while (next_blocks != NULL) {
				next_blocks->overwritten = 0;
				next_blocks = next_blocks->next;
			}
		}
		rw_ptr = 0;

		while (bytes_written < size) {
        if (current_block == NULL || current_block->overwritten == BLOCK_SIZE) {
            // Create a new block if the current one is full
					if ( current_block == NULL || current_block->next == NULL){

						  struct block *new_block = (struct block *)malloc(sizeof(struct block));
	            if (new_block == NULL) {
								  free(new_block);
	                ufs_error_code = UFS_ERR_NO_MEM;
	                return -1;
	            }
	            new_block->memory = (char *)malloc(BLOCK_SIZE);
	            if (new_block->memory == NULL) {
	                free(new_block);
	                ufs_error_code = UFS_ERR_NO_MEM;
	                return -1;
	            }
	            new_block->occupied = 0;
							new_block->overwritten = 0;
	            new_block->next = NULL;
	            new_block->prev = current_block;
							file->blocks_count++;

	            if (current_block != NULL) {
	                current_block->next = new_block;
	            } else {

									if (file->blocks_count == 1 ){
										file->block_list = new_block;
										file->last_block = new_block;
									}
									else{
										new_block->prev = file->last_block;
										file->last_block->next = new_block;
										file->last_block = new_block;
										file->last_block = new_block;
									}
	            }
							if ( current_block == file->last_block){
								file->last_block = new_block;
							}
	            current_block = new_block;
					}
					else {
						current_block = current_block->next;
					}
        }
        size_t remaining_space = BLOCK_SIZE - current_block->overwritten;
        size_t bytes_to_write = (size - bytes_written) < remaining_space ? (size - bytes_written) : remaining_space;
				memcpy(current_block->memory + current_block->overwritten, buf + bytes_written, bytes_to_write);
        current_block->overwritten += bytes_to_write;
				rw_ptr += bytes_to_write;
        bytes_written += bytes_to_write;
				if (current_block->overwritten - current_block->occupied > 0){
					current_block->occupied = current_block->overwritten;
				}
    }
		file_descriptors[fd - 1]->rw_ptr += rw_ptr;
		if (file_descriptors[fd - 1]->rw_ptr >= file->file_size) {
			file->file_size = file_descriptors[fd - 1]->rw_ptr;
		}
    return bytes_written;
}


ssize_t
ufs_read(int fd, char *buf, size_t size) {
    if (fd <= 0  || file_descriptors[fd - 1] == NULL) {
        ufs_error_code = UFS_ERR_NO_FILE;
				return -1;
    }
		if (file_descriptors[fd - 1]->flag == UFS_WRITE_ONLY){
			ufs_error_code = UFS_ERR_NO_PERMISSION;
			return -1;
		}
    struct file *file = file_descriptors[fd - 1]->file;
    if (file->refs <= 0) {
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    }

    struct block *current_block = file->block_list;
    size_t bytes_read = 0;
		int rw_ptr = file_descriptors[fd - 1]->rw_ptr;

		if (rw_ptr >= file->file_size) {
			rw_ptr = file->file_size;
			file_descriptors[fd - 1]->rw_ptr = file->file_size;
		}
		while(rw_ptr>=BLOCK_SIZE && current_block != NULL){
 			current_block->overwritten = BLOCK_SIZE;
			rw_ptr -= BLOCK_SIZE;
			current_block = current_block->next;
		}

		if (current_block != NULL){
			current_block->overwritten = rw_ptr;
			struct block *next_blocks = current_block->next;
			while (next_blocks) {
				next_blocks->overwritten = 0;
				next_blocks = next_blocks->next;
			}
		}

		rw_ptr = 0;
		while (bytes_read < size && current_block != NULL) {
			size_t remaining_data = current_block->occupied - current_block->overwritten;
			size_t bytes_to_read = (size - bytes_read) < remaining_data ? (size - bytes_read) : remaining_data;
			memcpy(buf + bytes_read, current_block->memory + current_block->overwritten , bytes_to_read);
			current_block->overwritten += bytes_to_read;
			rw_ptr += bytes_to_read;
			bytes_read += bytes_to_read;
			current_block = current_block->next;

		}

		file_descriptors[fd - 1]->rw_ptr += rw_ptr;
    return bytes_read;
}

int
ufs_resize(int fd, size_t new_size) {
	if (fd <= 0 || fd > file_descriptor_capacity || file_descriptors[fd - 1] == NULL) {

			ufs_error_code = UFS_ERR_NO_FILE;
			return -1;
	}

	if(file_descriptors[fd - 1]->flag == UFS_READ_ONLY){
		ufs_error_code = UFS_ERR_NO_PERMISSION;
		return -1;
	}
	if (new_size > MAX_FILE_SIZE){
		ufs_error_code = UFS_ERR_NO_MEM;
		return -1;
	}
	struct file *file = file_descriptors[fd - 1]->file;
	if (new_size > file->file_size) {
			struct block *current_block = file->block_list;
			while (current_block) {
				new_size -= BLOCK_SIZE;
				current_block = current_block->next;
				if(new_size <= 0){
					break;
				}
			}
			if ( new_size > 0){
				while (new_size > 0) {
					struct block *new_block = (struct block *)malloc(sizeof(struct block));
					if (new_block == NULL) {
							free(new_block);
							ufs_error_code = UFS_ERR_NO_MEM;
							return -1;
					}
					new_block->memory = (char *)malloc(BLOCK_SIZE);
					if (new_block->memory == NULL) {
							free(new_block);
							ufs_error_code = UFS_ERR_NO_MEM;
							return -1;
					}
					new_block->occupied = BLOCK_SIZE;
					new_block->overwritten = 0;
					new_block->next = NULL;
					new_block->prev = current_block;
					file->blocks_count++;
					if (current_block != NULL) {
							current_block->next = new_block;
					} else {
							if (file->blocks_count == 1 ){
								file->block_list = new_block;
								file->last_block = new_block;
							}
							else{
								new_block->prev = current_block;
								current_block->next = new_block;
								file->last_block = new_block;
							}
					}
					if ( current_block == file->last_block){
						file->last_block = new_block;
					}
					current_block = new_block;
					new_size -= BLOCK_SIZE;
				}
			}
	}
	else {
		struct block *current_block = file->block_list;
		int rw_ptr = new_size;
		while(rw_ptr>=BLOCK_SIZE && current_block != NULL){
 			current_block->overwritten = BLOCK_SIZE;
			rw_ptr -= BLOCK_SIZE;
			if (current_block->next == NULL)
				break;
			current_block = current_block->next;
		}
		if (current_block != NULL){
			current_block->overwritten = rw_ptr;
			current_block->occupied = rw_ptr;
			struct block *next_blocks = current_block->next;
			while (next_blocks!=NULL) {
				next_blocks->overwritten = 0;
				next_blocks->occupied = 0;
				next_blocks = next_blocks->next;
			}
		}

	}
	file->file_size = new_size;
	return 0;
}

int
ufs_close(int fd)
{
    // Check if the file descriptor is valid
    if (fd <= 0 || fd > file_descriptor_capacity || file_descriptors[fd - 1] == NULL) {

        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    }

    // Decrease the reference count of the associated file
    struct filedesc *filedesc = file_descriptors[fd - 1];
    struct file *file = filedesc->file;
    file->refs--;
		if (file->refs == 0){
			if (file->ghost) {
					if (file == file_list) {
							file_list = file->next;
					}
					if (file->prev != NULL) {
							file->prev->next = file->next;
					}
					if (file->next != NULL) {
							file->next->prev = file->prev;
					}
					// Free the file's memory
					struct block *block = file->block_list;
					while (block != NULL) {
							struct block *next_block = block->next;
							free(block->memory);
							free(block);
							block = next_block;
					}
					free(file->name);
					free(file);
				}
		}
    // If no more references to the file, remove it from the file list and free its memory


    // Clean up the file descriptor
		file_descriptors[fd - 1] = NULL;
		free(filedesc);
		file_descriptor_count--;

    return 0;
}

int
ufs_delete(const char *filename)
{
    // Check if the filename is valid
    if (filename == NULL || strlen(filename) == 0) {
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    }

    // Find the file in the file list
    struct file *file = file_list;
    while (file != NULL) {
        if (strcmp(file->name, filename) == 0) {
							file->ghost = true;
							if (file->refs == 0){
                if (file == file_list) {
                    file_list = file->next;
                }
                if (file->prev != NULL) {
                    file->prev->next = file->next;
                }
                if (file->next != NULL) {
                    file->next->prev = file->prev;
                }

                // Free the file's memory
                struct block *block = file->block_list;
                while (block != NULL) {
                    struct block *next_block = block->next;
                    free(block->memory);
                    free(block);
                    block = next_block;
                }
                free(file->name);
                free(file);
							}
            return 0;
        }
        file = file->next;
    }

    // File not found
    ufs_error_code = UFS_ERR_NO_FILE;
    return -1;
}

void
ufs_destroy(void)
{
    // Clean up the file list
    struct file *file = file_list;
    while (file != NULL) {
        struct file *next_file = file->next;

        // Free the file's memory
        struct block *block = file->block_list;
        while (block != NULL) {
            struct block *next_block = block->next;
            free(block->memory);
            free(block);
            block = next_block;
        }
        free(file->name);
        free(file);

        file = next_file;
    }

    // Clean up the file descriptor array
    for (int i = 0; i < file_descriptor_capacity; i++) {
        if (file_descriptors[i] != NULL) {
            free(file_descriptors[i]);
        }
    }
    free(file_descriptors);
    file_descriptors = NULL;
    file_descriptor_count = 0;
    file_descriptor_capacity = 0;
}
