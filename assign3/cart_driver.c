////////////////////////////////////////////////////////////////////////////////
//
//  File           : cart_driver.c
//  Description    : This is the implementation of the standardized IO functions
//                   for used to access the CART storage system.
//
//  Author         : Mayank Makwana
//  Last Modified  : 10/14/16
//

// Includes
#include <stdlib.h>
#include <string.h>

// Project Includes
#include <cart_driver.h>
#include <cart_controller.h>
#include <cart_cache.h>

//
// Implementation

// Function Declarations
CartXferRegister generate_encoded_opcode(CartXferRegister, CartXferRegister, CartXferRegister, CartXferRegister); //generates opcode
int hash(char*); //hashed file path
int extract_opcode_response(CartXferRegister); //extracts opcode
int run_opcode(CartXferRegister, void*); //runs opcode

////////////////////////////////////////////////////////////////////////////////
//
// Function     : generate_encoded_opcode
// Description  : Creates opcode for passing to file system
//
// Inputs       : First key, second key, cartridge, and frame
// Outputs      : 64 bit opcode

CartXferRegister generate_encoded_opcode(CartXferRegister ky_1, CartXferRegister ky_2, CartXferRegister ct_1, CartXferRegister fm_1) {
    CartXferRegister base = 0; //empty 64bit num
    ky_1 = ky_1 << 56; //sets ky_1 to last 8 bits
    ky_2 = ky_2 << 48; //set ky_2 to next 8 bits
    ct_1 = ct_1 << 31; //sets ct_1 to next 16 bits
    fm_1 = fm_1 << 15; //sets fm_1 to next 16 bits

    base = base | ky_1; //adds ky_1
    base = base | ky_2; //adds ky_2
    base = base | ct_1; //adds ct_1
    base = base | fm_1; //adds fm_1
   
    return base; //returns number
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : extract_opcode_response
// Description  : Determines if opcode succeeded or failed
//
// Inputs       : Opcode response from cart system
// Outputs      : 0 if successful, -1 if failure

int extract_opcode_response(CartXferRegister response) {
    response = response << 16; //clears last bits in response
    response = response >> 63; //moves return bit to beginning
    if(response == 0) { //if return bit is 0
        return 0; //return success
    } else {
        return -1; //return failure
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : run_opcode
// Description  : Runs command to cart system
//
// Inputs       : Opcode and buffer
// Outputs      : 0 if successful, -1 if failure

int run_opcode(CartXferRegister opcode, void *buf) {
    CartXferRegister response; //response from running opcode

    if (buf == NULL) { //if no buffer passed
        response = cart_io_bus(opcode, 0); //run command without buffer
    } else { //if buffer passed
        response = cart_io_bus(opcode, buf); //run command with buffer
    }
    return extract_opcode_response(response); //return success or failure by checking extracted opcode
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : hash
// Description  : Hashed file name for quick asses to fd
//
// Inputs       : File path
// Outputs      : location of fd

int hash(char *str) {
    unsigned long hash = 0; //initial hash to 0
    unsigned long factor = 13; //random factor
    unsigned char *ustr = (unsigned char *) str; //converts input to usigned version

    while(*ustr != '\0') { //while the end of the string isn't reached
        hash = (hash * factor + *ustr) % FILES_SIZE; //creates random location using math
        ustr++; //keep going
    }
    
    return hash % FILES_SIZE; //return hash in range of files_size
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_poweron
// Description  : Startup up the CART interface, initialize filesystem
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int32_t cart_poweron(void) {
    int i, j, k, start, start_cache, load, clear; //temporary variables
    
    start = run_opcode(generate_encoded_opcode(CART_OP_INITMS, 0, 0, 0), NULL); //initialize cart system
    start_cache = init_cart_cache();
    if (start == -1 || start_cache == -1) { return(-1); } //if cart system fails to initialize, return -1

    file_system.is_on = 1; //turns on file system
    file_system.current_handle = 0; //sets initial file handle
    file_system.cart_to_use = 0; //sets initial cart
    file_system.frame_to_use = 0; //sets initial frame

    for(i = 0; i < CART_MAX_CARTRIDGES; i++) { //clears all cartridges
        load = run_opcode(generate_encoded_opcode(CART_OP_LDCART, 0, i, 0), NULL); //loads cartridge
        if (load == -1) { return(-1);  } //if loading fails, return -1
        clear = run_opcode(generate_encoded_opcode(CART_OP_BZERO, 0, 0, 0), NULL); //clear cartridge
        if(clear == -1) { return(-1); } //if cartridge fails to load, return -1
        file_system.last_cart_loaded = i;
    }
    
    for(i = 0; i < FILES_SIZE; i++) { //creates empty files and file handles
        File temp;  //empty file structure for when new files are created
        temp.name[0] = '0';
        file_system.files[i] = temp; //add empty file to system
        file_system.all_handles[i] = -1; //set handles to -1
    }
    
    for(j = 0; j < CART_MAX_CARTRIDGES; j++) { //iterates through carts
        for(k = 0; k < CART_CARTRIDGE_SIZE; k++) { //iterates through frames
            file_system.visited[j][k] = 0; // sets each frame to 0 to signal unvisited
        }
    }

    return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_poweroff
// Description  : Shut down the CART interface, close all files
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int32_t cart_poweroff(void) {
    int i; //iterating variable 
    int off = run_opcode(generate_encoded_opcode(CART_OP_POWOFF, 0, 0, 0), NULL); //shuts down cart system
    int close_cache = close_cart_cache(); //closes cache

    for(i = 0; i < FILES_SIZE; i++) { //close all files
        file_system.files[i].is_open = 0; //0 is close
    }
    
    if(off == -1 || close_cache == -1) { return(-1); } //if shutdown fails, return -1
    file_system.is_on = 0; //shutdown file system
    // Return successfully
	return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_open
// Description  : This function opens the file and returns a file handle
//
// Inputs       : path - filename of the file to open
// Outputs      : file handle if successful, -1 if failure

int16_t cart_open(char *path) {
    int i, hashed = hash(path); //initiate i and hashes path to get index for array
    int file_handle = file_system.all_handles[hashed]; //gets file handle from hashed list
    
    if(file_handle == -1) { //if file does not exist create a new one
        file_handle = file_system.current_handle; //gets a new, unused file handle
        File new_file; //creates a new file object
        strcpy(new_file.name, path); //copies file path
        new_file.handle = file_handle; //adds handle
        new_file.size = 0; //sets initial size
        new_file.current_position = -1; //sets current position
        new_file.is_open = 1; //sets open to 1
        new_file.data[0].cart = file_system.cart_to_use; //initial cart is next available
        new_file.data[0].frame = file_system.frame_to_use; // initial frame is next available
        for(i = 1; i < CART_CARTRIDGE_SIZE; i++) {
            new_file.data[i].cart = -1; //all other carts are -1, signifying unused
            new_file.data[i].frame = -1; //all other frames are -1, signifying unused
        }
        file_system.files[file_handle] = new_file; //adds new file to file system 
        file_system.current_handle = file_handle + 1; //creates new unused file handle for next file
        file_system.all_handles[hashed] = file_handle; //adds handle to hashed list for O(1) access to handle based on path name    
        file_system.frame_to_use++; //increments frame
        if(file_system.frame_to_use == CART_CARTRIDGE_SIZE) { //if end of cart
            file_system.cart_to_use++; //go to next cart
            file_system.frame_to_use = 0; //set to first frame in cart
        }
    } else if(file_system.files[file_handle].is_open == 0) { //if file isn't open
        file_system.files[file_handle].is_open = 1; //open file
    } else { //any other case
        return(-1); //fail
    }

	// THIS SHOULD RETURN A FILE HANDLE
	return (file_handle);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_close
// Description  : This function closes the file
//
// Inputs       : fd - the file descriptor
// Outputs      : 0 if successful, -1 if failure

int16_t cart_close(int16_t fd) {
    if(file_system.files[fd].name[0] == '0' || file_system.files[fd].is_open != 1) { //checks if file exists or is already closed
        return(-1);
    }
    
    file_system.files[fd].is_open = 0; //closes file system

    // Return successfully
	return (0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_read
// Description  : Reads "count" bytes from the file handle "fh" into the 
//                buffer "buf"
//
// Inputs       : fd - filename of the file to read from
//                buf - pointer to buffer to read into
//                count - number of bytes to read
// Outputs      : bytes read if successful, -1 if failure

int32_t cart_read(int16_t fd, void *buf, int32_t count) {
    File *current = &file_system.files[fd]; //gets current file
    int read_location_frame = current->current_position / (CART_FRAME_SIZE - 1); //gets starting frame
    int read_location_bytes = current->current_position % (CART_FRAME_SIZE - 1); //gets position inside frame
    char read_in[CART_FRAME_SIZE]; //frame to read from system
    int i, response; //iterator and handles response
    char *char_buf = (char *) buf; //converts void buf to char buf
    char *stored_data; //stores data read
    cache_node *read_cache; //reads data from cache

    stored_data = (char *)malloc(sizeof(char)*count); //allocates data read to count size
    read_cache = get_cart_cache(current->data[read_location_frame].cart, current->data[read_location_frame].frame); //get data from cache
    
    if(read_cache != NULL) { //if data in cache, get data from cache
        memcpy(read_in, read_cache->data, CART_FRAME_SIZE); //copy data into read_in
    } else { //if no data in cache, get data from cart
        response = run_opcode(generate_encoded_opcode(CART_OP_RDFRME, 0, 0, current->data[read_location_frame].frame), &read_in); //gets frame
        if(response == -1) return(-1); //if call fails
    }

    for(i = 0; i < count; i++) { //iterates through bytes
        if(read_location_bytes == CART_FRAME_SIZE-1){ //if finished reading frame
            read_location_frame++; //move to next frame
            read_location_bytes = 0; //start at pos 0 in frame
            read_cache = get_cart_cache(current->data[read_location_frame].cart, current->data[read_location_frame].frame); //get data from cache
            
            if(read_cache != NULL) { //if data in cache
                memcpy(read_in, read_cache->data, CART_FRAME_SIZE); //copy data into read in
            } else { //if no data in cache
                response = run_opcode(generate_encoded_opcode(CART_OP_RDFRME, 0, 0, current->data[read_location_frame].frame), &read_in);//get frame
                if(response == -1) return(-1); //check if call succeeded
            }
        }
        stored_data[i] = read_in[read_location_bytes]; //add frame data to stored data
        read_location_bytes++; //go to next byte
    }

    memcpy(char_buf, stored_data, count); //copy info from stored data to buffer
    free(stored_data); //release memory

    // Return successfully
	return (count);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_write
// Description  : Writes "count" bytes to the file handle "fh" from the 
//                buffer  "buf"
//
// Inputs       : fd - filename of the file to write to
//                buf - pointer to buffer to write from
//                count - number of bytes to write
// Outputs      : bytes written if successful, -1 if failure

int32_t cart_write(int16_t fd, void *buf, int32_t count) {
    File *current = &file_system.files[fd]; //points to current file
    int write_location_frame, write_location_bytes; //location to write and excess bytes
    char read_in[CART_FRAME_SIZE] = { [0 ... CART_FRAME_SIZE-1] = '\0'}; //data read in from frame
    char holding_buffer[CART_FRAME_SIZE] = { [0 ... CART_FRAME_SIZE-1] = '\0' }; //holds data to insert into frame
    char *char_buf = (char *) buf; //converts void buf to char buf
    int holding_buffer_count = 0; //counts index for holding_buffer
    int response, i; //response for cart and iterating vari
    cache_node *read_cache; //read data from cache

    if(file_system.files[fd].name[0] == '0' || file_system.files[fd].is_open != 1) { //checks if file exists or is already closed
        return(-1);
    }
    
    for(i = 0; i < count; i++) { //iterates through bytes to write
        write_location_frame = current->current_position / (CART_FRAME_SIZE - 1); //gets frame to write
        write_location_bytes = current->current_position % (CART_FRAME_SIZE - 1); //gets position to write
        holding_buffer[holding_buffer_count] = char_buf[i]; //gets writing data
        if((holding_buffer_count + write_location_bytes + 1) == (CART_FRAME_SIZE - 1) || i == (count - 1)) { //if max data reached or end of file
            if(file_system.last_cart_loaded != current->data[write_location_frame].cart) { //checks if cart is open
                response = run_opcode(generate_encoded_opcode(CART_OP_LDCART, 0, current->data[write_location_frame].cart, 0), NULL); //opens cart
                if(response == -1) return(-1); //if failed, return -1
                file_system.last_cart_loaded = current->data[write_location_frame].cart; //sets last cart loaded
            }
            
            read_cache = get_cart_cache(current->data[write_location_frame].cart, current->data[write_location_frame].frame); //get data from cache
            if(read_cache != NULL) { //if cache has data
                memcpy(read_in, read_cache->data, CART_FRAME_SIZE); //copy data into cache
            } else if(file_system.visited[current->data[write_location_frame].cart][current->data[write_location_frame].frame]){ //if unvisited
                response = run_opcode(generate_encoded_opcode(CART_OP_RDFRME, 0, 0, current->data[write_location_frame].frame), &read_in); //read
                if(response == -1) return(-1);//checks if successful
            }

            memcpy(&read_in[write_location_bytes], holding_buffer, holding_buffer_count+1);//copies new data
            response = run_opcode(generate_encoded_opcode(CART_OP_WRFRME, 0, 0, current->data[write_location_frame].frame), &read_in);//writes frame
            put_cart_cache(current->data[write_location_frame].cart, current->data[write_location_frame].frame, &read_in); //add data to cache
            file_system.visited[current->data[write_location_frame].cart][current->data[write_location_frame].frame] = 1; //mark data as visited

            if(response == -1) return(-1); //checks if successful

            current->current_position += (holding_buffer_count + 1); //update current position
            if(current->current_position + 1 > current->size) { //checks if size changes
                current->size = current->current_position + 1; //increases size
            }
            
            holding_buffer_count = -1; //resets counter

            if(current->current_position % (CART_FRAME_SIZE-1) == 0) { //checks if end of frame
                if(current->current_position + 1 == current->size) { //if new frame needs to be allocated
                    current->data[write_location_frame+1].cart = file_system.cart_to_use; //go to next cart to use
                    current->data[write_location_frame+1].frame = file_system.frame_to_use; //go to next frame to use
                    file_system.frame_to_use++; //increment frame
                    if(file_system.frame_to_use == CART_CARTRIDGE_SIZE) { //if end of cart
                        file_system.cart_to_use++; //go to new cart
                        file_system.frame_to_use = 0; //go to first frame in cart 
                    }
                }
            }
        }
        holding_buffer_count++; //increment counter
    }

    // Return successfully
	return (count);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_read
// Description  : Seek to specific point in the file
//
// Inputs       : fd - filename of the file to write to
//                loc - offfset of file in relation to beginning of file
// Outputs      : 0 if successful, -1 if failure

int32_t cart_seek(int16_t fd, uint32_t loc) {
	File *current = &file_system.files[fd]; //current file
    
    if(file_system.files[fd].name[0] == '0' || file_system.files[fd].is_open != 1) { //checks if file exists or is already closed
        return(-1);
    }
   
    if(loc+1 > current->size) { //checks if location is in bounds 
        return(-1);
    }
    
    current->current_position = loc; //sets position to new location
    
    // Return successfully
	return (0);
}
