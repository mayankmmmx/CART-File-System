/////////////////////////////////////////////////////////////////////////////////
//
//  File           : cart_cache.c
//  Description    : This is the implementation of the cache for the CART
//                   driver.
//
//  Author         : Mayank Makwana
//  Last Modified  : 11/22/2016 
//

// Includes
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
// Project includes
#include <cart_cache.h>
#include <cart_controller.h>
#include <cmpsc311_log.h>
// Defines

//
// Functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : set_cart_cache_size
// Description  : Set the size of the cache (must be called before init)
//
// Inputs       : max_frames - the maximum number of items your cache can hold
// Outputs      : 0 if successful, -1 if failure

int set_cart_cache_size(uint32_t max_frames) {
    cache.max_cache_size = max_frames; //sets max cache size;
    return(0); //successfully sets cache size
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : init_cart_cache
// Description  : Initialize the cache and note maximum frames
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int init_cart_cache(void) {
    int i, j; //iterating variables
    cache.max_cache_size = (cache.max_cache_size == 0) ? DEFAULT_CART_FRAME_CACHE_SIZE : cache.max_cache_size;
    
    for(i = 0; i < CART_MAX_CARTRIDGES; i++) { //iterate through carts
        for(j = 0; j < CART_CARTRIDGE_SIZE; j++) //iterate through frames
            cache.filled_cache_frames[i][j] = NULL; //set everything in cache to null
    }
    
    cache.current_cache_size = 0; //initially cache is empty
    cache.head = NULL; //sets cache head to null
    cache.tail = NULL; //sets cache tail to null

    return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : close_cart_cache
// Description  : Clear all of the contents of the cache, cleanup
//
// Inputs       : none
// Outputs      : o if successful, -1 if failure

int close_cart_cache(void) {
    cache_node *current = cache.head; //current node
    cache_node *prev; //previous node in list

    while(current != NULL) { //go through every node in list to delete
        prev = current->prev; // set prev node to the prev of current
        delete_cart_cache(current->cart, current->frame); //delete current node 
        current = prev; //set current to prev node
    }
   
    cache.head = NULL; //set cache head to null
    cache.tail = NULL; //set cache tail to null
    
    return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : put_cart_cache
// Description  : Put an object into the frame cache
//
// Inputs       : cart - the cartridge number of the frame to cache
//                frm - the frame number of the frame to cache
//                buf - the buffer to insert into the cache
// Outputs      : 0 if successful, -1 if failure

int put_cart_cache(CartridgeIndex cart, CartFrameIndex frm, void *buf)  {
    if(cache.max_cache_size <= 0) return(0); //if cache size is non-positive, cache is not used, therefore treat program as normal
    
    cache_node *node = cache.filled_cache_frames[cart][frm]; //get data from cache storage
    char *charbuf = (char *) buf; //converts text buffer to string 
    
    if(node == NULL) { //if data not in cache
        node = (cache_node *) malloc(sizeof(cache_node)); //create node of correct size 
        memcpy(node->data, charbuf, CART_FRAME_SIZE); // copy data from charbuf into node
        node->cart = cart; //set cart
        node->frame = frm; //set frame
        node->prev = NULL; //prev to null
        node->next = NULL; //next to null
        
        if(cache.current_cache_size == 0) { //if cache is empty
            cache.head = node; //make node head
        } else if(cache.current_cache_size < cache.max_cache_size) { //if cache has room
            node->next = cache.tail; //point node next to current tail
            cache.tail->prev = node; //point tail prev to node
        } else { //if cache has no room
            delete_cart_cache(cache.head->cart, cache.head->frame); //delete top of cache
            if(cache.current_cache_size == 0) { //if cache is empty
                cache.head = node; //make node head
            } else { //if not emptyy
                node->next = cache.tail; //point node next tp current tail
                cache.tail->prev = node; //point tail prev to node
            }
        }
        
        cache.tail = node; //set tail to node
        cache.current_cache_size++; //increase current cache size
        cache.filled_cache_frames[cart][frm] = node; //add node to the filled frames
    } else { //if data is already in cache
        if(cache.current_cache_size > 1 && (node != cache.tail)) { //if cache size is greather than 1 or node isnt at end
            if(node == cache.head) { // if node is head
                cache.head = cache.head->prev; //make the head's previous node the new head
                cache.head->next->next = cache.tail; //set node's (old head) next to the tail
                cache.tail->prev = cache.head->next; //set the tail's prev to node (old head)
                cache.tail = cache.head->next; //set the tail to node (old head)
                cache.tail->prev = NULL; //tail prev is null
                cache.head->next = NULL; //head next is null
            } else { //if node is somewhere else in the cache
                node->prev->next = node->next; //set the node prev next to node next
                node->next->prev = node->prev; //set the node next prev to node prev
                node->next = cache.tail; //set node next to tail
                cache.tail->prev = node; //set tail prev to node
                node->prev = NULL; //set node prev to null
                cache.tail = node; //make node the tail
            }
        }
        memcpy(node->data, charbuf, CART_FRAME_SIZE); //copy new data from buf into node
    }

    return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : get_cart_cache
// Description  : Get an frame from the cache (and return it)
//
// Inputs       : cart - the cartridge number of the cartridge to find
//                frm - the  number of the frame to find
// Outputs      : pointer to cached frame or NULL if not found

void * get_cart_cache(CartridgeIndex cart, CartFrameIndex frm) {
    return(cache.filled_cache_frames[cart][frm]); //get that in cache (NULL if no data)
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : delete_cart_cache
// Description  : Remove a frame from the cache
//
// Inputs       : cart - the cart number of the frame to remove from cache
//                blk - the frame number of the frame to remove from cache
// Outputs      : 0 if successfully delete; -1 if failed.

int delete_cart_cache(CartridgeIndex cart, CartFrameIndex blk) {
    cache_node *delete = cache.filled_cache_frames[cart][blk]; //gets node to delete
    
    if(delete == NULL) { //if node to delete is NULL
        return(-1); //cannot delete
    } else { //if there exists a node to delete
        if(cache.current_cache_size > 1) { //if there is more than one node in cache
            if(delete == cache.head) { //if node to delete is head
                cache.head = cache.head->prev; //set new head to head prev
                cache.head->next->prev = NULL; //set node to delete prev to null
                cache.head->next = NULL; //set head next to null 
            } else if(delete == cache.tail) { //if node to delete is tail
                cache.tail = cache.tail->next; //set new tail to tail next
                cache.tail->prev->next = NULL; //set node to delete next to null
                cache.tail->prev = NULL; //set tail prev to null
            } else { //if node to delete is anywhere else in cache
                delete->prev->next = delete->next; //set delete prev next to delete next
                delete->next->prev = delete->prev; //set delete next prev to delete prev
                delete->prev = NULL; //set delete prev to null
                delete->next = NULL; //set delete next to null
            }
        } 
        free(delete); //free the node to delete from memory
        delete = NULL; //set delete pointer to null
        cache.filled_cache_frames[cart][blk] = NULL; //remove the node from cache
        cache.current_cache_size--; //decrement the current size of cache
    }

    return(0);
}

//
// Unit test

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cartCacheUnitTest
// Description  : Run a UNIT test checking the cache implementation
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int cartCacheUnitTest(void) {
    int start, close, i, r_cart, r_frame, op, put; //temp variables 
    cache_node *read; //read in node for cache
    char *data[5][5] = { //sample data to test cache with
        {"hello", "how", "are", "you", "today"},
        {"im", "good", "what", "about", "yourself"},
        {"thanks", "for", "asking", "doing", "well"},
        {"welcome", "to", "the", "big", "jungle"},
        {"i", "was", "like", "hey", "whats"}
    };

    start = init_cart_cache(); //initialize cache
    if(start == -1) return(-1); //if init fails, return fail
    
    for(i = 0; i < 10000; i++) { //10000 tests
        op = rand() % 2; //gets random operation from 0-1; 0 = get, 1 = put
        r_cart = rand() % 5; //gets random cart from 0-4
        r_frame = rand() % 5; //gets random frame from 0-4
        
        if(op == 0) { //if op is 0, try get command
            read = get_cart_cache(r_cart, r_frame); //get random data from cache
            if(read != NULL) { //if data exists
                if(strcmp(read->data, data[r_cart][r_frame])) return(-1); //compare cache with sample data, fail if data is different
            }
        } else { //if op is 1, try put command
            put = put_cart_cache(r_cart, r_frame, data[r_cart][r_frame]); //insert data into frame
            if(put == -1) return(-1); //if put fails, return fail
        }
    }
    
    close = close_cart_cache(); //close cache
    if(close == -1) return(-1); //if close fails, return -1

	logMessage(LOG_OUTPUT_LEVEL, "Cache unit test completed successfully."); //output sucess message
	return(0); //all tests succeeded!
}
