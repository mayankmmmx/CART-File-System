#ifndef CART_FRAME_CACHE_INCLUDED
#define CART_FRAME_CACHE_INCLUDED

////////////////////////////////////////////////////////////////////////////////
//
//  File           : cart_frame_cache.h
//  Description    : This is the header file for the implementation of the
//                   frame cache for the cartridge memory system driver.
//
//  Author         : Patrick McDaniel
//  Last Modified  : Sun Oct 16 07:59:59 EDT 2016
//

// Includes
#include <cart_controller.h>
#include <stdint.h>
// Defines
#define DEFAULT_CART_FRAME_CACHE_SIZE 1024  // Default size for cache

//LRU STRUCT
typedef struct cache_node {
    char data[CART_FRAME_SIZE]; //data
    int cart; //cart data is stored in
    int frame; //frame data is stored in
    struct cache_node* next; //next in list
    struct cache_node* prev; //previous in list
} cache_node;

//CACHE STRUCT
typedef struct cach_struct {    
    int max_cache_size; //max size of cache
    cache_node* filled_cache_frames[CART_MAX_CARTRIDGES][CART_CARTRIDGE_SIZE]; //frames stored in cache
    int current_cache_size; //current size of cache
    cache_node* head; //top of cache
    cache_node* tail; //bottom of cache
} Cache;

Cache cache; //new cache

///
// Cache Interfaces
int set_cart_cache_size(uint32_t max_frames);
	// Set the size of the cache (must be called before init)

int init_cart_cache(void);
	// Initialize the cache 

int close_cart_cache(void);
	// Clear all of the contents of the cache, cleanup

int put_cart_cache(CartridgeIndex cart, CartFrameIndex frm, void *frame);
	// Put an object into the object cache, evicting other items as necessary

void * get_cart_cache(CartridgeIndex dsk, CartFrameIndex blk);
	// Get an object from the cache (and return it)

int delete_cart_cache(CartridgeIndex cart, CartFrameIndex blk);
    // Delete object from the cache
//
// Unit test

int cartCacheUnitTest(void);
	// Run a UNIT test checking the cache implementation

#endif
