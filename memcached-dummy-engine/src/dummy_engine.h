/*
 * dummy_engine.c
 *
 *  Created on: 15/03/2012
 *      Author: fviale
 */

#ifndef DUMMY_ENGINE_C_
#define DUMMY_ENGINE_C_

	#include <memcached/engine.h>
	#include <memcached/util.h>
	#include <memcached/visibility.h>

	typedef struct {
	   void *key;
	   size_t nkey;
	   void *data;
	   size_t ndata;
	   int flags;
	   rel_time_t exptime;
	}t_dummy_ng_item;

	typedef struct {
	   size_t cache_max_size;
	   size_t block_size_max;
	   size_t chunk_size;
	}t_dummy_ng_config;


	typedef struct {
		ENGINE_HANDLE_V1 engine;
		GET_SERVER_API get_server_api;
		t_dummy_ng_config config;
	}t_dummy_ng;

	MEMCACHED_PUBLIC_API ENGINE_ERROR_CODE create_instance(uint64_t interface, GET_SERVER_API get_server_api, ENGINE_HANDLE **handle);

#endif /* DUMMY_ENGINE_C_ */
