#ifndef CART_DRIVER_INCLUDED
#define CART_DRIVER_INCLUDED

////////////////////////////////////////////////////////////////////////////////
//
//  File           : cart_driver.h
//  Description    : This is the header file for the standardized IO functions
//                   for used to access the CART storage system.
//
//  Author         : Patrick McDaniel
//  Last Modified  : Thu Sep 15 15:05:53 EDT 2016
//

// Include files
#include <stdint.h>
// Defines
#define CART_MAX_TOTAL_FILES 1024 // Maximum number of files ever
#define CART_MAX_PATH_LENGTH 128 // Maximum length of filename length
#define FILES_SIZE CART_MAX_TOTAL_FILES * 5 //Huge size for ample space in hash table

#include "cart_controller.h"

//FILE STRUCT
typedef struct file_structure {
    char name[CART_MAX_PATH_LENGTH]; //name of file
    int16_t handle; //file handle
    int32_t size; //file size
    int32_t current_position; //location in file
    int     is_open; //whether or not file is open
    struct data_structure { //Data info of file
        int16_t cart; //cart
        int16_t frame; //frame
    } data[CART_CARTRIDGE_SIZE];
} File;

//FILE SYSTEM STRUCT
typedef struct file_system_structure {
    int16_t cart_to_use; //cart to use for next file
    int16_t frame_to_use; //frame to use for next file
    int16_t last_cart_loaded; //last loaded cart
    int is_on; //is system on
    int current_handle; //handle for next file
    File files[FILES_SIZE]; //files in file system
    int all_handles[FILES_SIZE]; //all handles in file system
    int visited[CART_MAX_CARTRIDGES][CART_CARTRIDGE_SIZE];
} File_System;

File_System file_system; //new file system

//
// Interface functions

int32_t cart_poweron(void);
	// Startup up the CART interface, initialize filesystem

int32_t cart_poweroff(void);
	// Shut down the CART interface, close all files

int16_t cart_open(char *path);
	// This function opens the file and returns a file handle

int16_t cart_close(int16_t fd);
	// This function closes the file

int32_t cart_read(int16_t fd, void *buf, int32_t count);
	// Reads "count" bytes from the file handle "fh" into the buffer  "buf"

int32_t cart_write(int16_t fd, void *buf, int32_t count);
	// Writes "count" bytes to the file handle "fh" from the buffer  "buf"

int32_t cart_seek(int16_t fd, uint32_t loc);
	// Seek to specific point in the file


#endif


