#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <memcached/config_parser.h>

#include "collections/dictionary.h"

#include "dummy_engine.h"

static ENGINE_ERROR_CODE dummy_ng_initialize(ENGINE_HANDLE* , const char* config_str);
static void dummy_ng_destroy(ENGINE_HANDLE*, const bool force);

static ENGINE_ERROR_CODE dummy_ng_allocate(ENGINE_HANDLE* , const void* cookie, item **item, const void* key,
											const size_t nkey, const size_t nbytes, const int flags, const rel_time_t exptime);
static bool dummy_ng_get_item_info(ENGINE_HANDLE *, const void *cookie, const item* item, item_info *item_info);
static ENGINE_ERROR_CODE dummy_ng_store(ENGINE_HANDLE* , const void *cookie, item* item, uint64_t *cas,
											ENGINE_STORE_OPERATION operation, uint16_t vbucket);
static void dummy_ng_item_release(ENGINE_HANDLE* , const void *cookie, item* item);
static ENGINE_ERROR_CODE dummy_ng_get(ENGINE_HANDLE* , const void* cookie, item** item, const void* key, const int nkey, uint16_t vbucket);

/*************************** Dummy Functions ***************************/
static const engine_info* dummy_ng_get_info(ENGINE_HANDLE* );
static ENGINE_ERROR_CODE dummy_ng_item_delete(ENGINE_HANDLE* , const void* cookie, const void* key, const size_t nkey, uint64_t cas, uint16_t vbucket);
static ENGINE_ERROR_CODE dummy_ng_get_stats(ENGINE_HANDLE* , const void* cookie, const char* stat_key, int nkey, ADD_STAT add_stat);
static ENGINE_ERROR_CODE dummy_ng_flush(ENGINE_HANDLE* , const void* cookie, time_t when);
static void dummy_ng_reset_stats(ENGINE_HANDLE* , const void *cookie);
static ENGINE_ERROR_CODE dummy_ng_unknown_command(ENGINE_HANDLE* , const void* cookie, protocol_binary_request_header *request, ADD_RESPONSE response);
static void dummy_ng_item_set_cas(ENGINE_HANDLE *, const void *cookie, item* item, uint64_t val);
/**********************************************************************/

void dummy_ng_dummp(int signal);


t_dictionary *cache;

MEMCACHED_PUBLIC_API ENGINE_ERROR_CODE create_instance(uint64_t interface, GET_SERVER_API get_server_api, ENGINE_HANDLE **handle) {

	/*
	 * Verify that the interface from the server is one we support. Right now
	 * there is only one interface, so we would accept all of them (and it would
	 * be up to the server to refuse us... I'm adding the test here so you
	 * get the picture..
	 */
	if (interface == 0) {
		return ENGINE_ENOTSUP;
	}

	/*
	 * Allocate memory for the engine descriptor. I'm no big fan of using
	 * global variables, because that might create problems later on if
	 * we later on decide to create multiple instances of the same engine.
	 * Better to be on the safe side from day one...
	 */
	t_dummy_ng *engine = calloc(1, sizeof(t_dummy_ng));
	if (engine == NULL) {
		return ENGINE_ENOMEM;
	}

	engine->get_server_api = get_server_api;

	/*
	 * We're going to implement the first version of the engine API, so
	 * we need to inform the memcached core what kind of structure it should
	 * expect
	 */
	engine->engine.interface.interface = 1;

	/*
	 * Map the API entry points to our functions that implement them.
	 */
	engine->engine.initialize = dummy_ng_initialize;
	engine->engine.destroy = dummy_ng_destroy;
	engine->engine.get_info = dummy_ng_get_info;
	engine->engine.allocate = dummy_ng_allocate;
	engine->engine.remove = dummy_ng_item_delete;
	engine->engine.release = dummy_ng_item_release;
	engine->engine.get = dummy_ng_get;
	engine->engine.get_stats = dummy_ng_get_stats;
	engine->engine.reset_stats = dummy_ng_reset_stats;
	engine->engine.store = dummy_ng_store;
	engine->engine.flush = dummy_ng_flush;
	engine->engine.unknown_command = dummy_ng_unknown_command;
	engine->engine.item_set_cas = dummy_ng_item_set_cas;
	engine->engine.get_item_info = dummy_ng_get_item_info;

	/* Pass the handle back to the core */
	*handle = (ENGINE_HANDLE*) engine;

	cache = dictionary_create(free);

	return ENGINE_SUCCESS;
}

static ENGINE_ERROR_CODE dummy_ng_initialize(ENGINE_HANDLE* handle, const char* config_str){
	t_dummy_ng *engine = (t_dummy_ng*)handle;

   if (config_str != NULL) {
	  struct config_item items[] = {
		 { .key = "cache_size",
		   .datatype = DT_SIZE,
		   .value.dt_size = &engine->config.cache_max_size },
		 { .key = "chunk_size",
		   .datatype = DT_SIZE,
		   .value.dt_size = &engine->config.chunk_size },
		 { .key = "item_size_max",
		   .datatype = DT_SIZE,
		   .value.dt_size = &engine->config.block_size_max },
		 { .key = NULL}
	  };

	  parse_config(config_str, items, NULL);
   }

   signal(SIGUSR1, dummy_ng_dummp);

   return ENGINE_SUCCESS;
}

static void dummy_ng_destroy(ENGINE_HANDLE* handle, const bool force){
	free(handle);
}


static ENGINE_ERROR_CODE dummy_ng_allocate(ENGINE_HANDLE *handler, const void* cookie, item **item, const void* key,
											const size_t nkey, const size_t nbytes, const int flags, const rel_time_t exptime){

	t_dummy_ng_item *it = malloc( sizeof(t_dummy_ng_item) );

	if (it == NULL) {
		return ENGINE_ENOMEM;
	}

	it->flags = flags;
	it->exptime = exptime;
	it->nkey = nkey;
	it->ndata = nbytes;
	it->key = malloc(nkey);
	it->data = malloc(nbytes);

	memcpy(it->key, key, nkey);
	*item = it;

	return ENGINE_SUCCESS;
}

static void dummy_ng_item_release(ENGINE_HANDLE *handler, const void *cookie, item* item){
	t_dummy_ng_item *it = (t_dummy_ng_item*)item;
	free(it->key);
	free(it->data);
	free(it);
}

static bool dummy_ng_get_item_info(ENGINE_HANDLE *handler, const void *cookie, const item* item, item_info *item_info){
	t_dummy_ng_item *it = (t_dummy_ng_item*)item;

	if (item_info->nvalue < 1) {
	  return false;
	}

	item_info->cas = 0; 		/* Not supported */
	item_info->clsid = 0; 		/* Not supported */
	item_info->exptime = it->exptime;
	item_info->flags = it->flags;
	item_info->key = it->key;
	item_info->nkey = it->nkey;
	item_info->nbytes = it->ndata; 	/* Total length of the items data */
	item_info->nvalue = 1; 			/* Number of fragments used */
	item_info->value[0].iov_base = it->data; /* pointer to fragment 1 */
	item_info->value[0].iov_len = it->ndata; /* Length of fragment 1 */

	return true;
}

static ENGINE_ERROR_CODE dummy_ng_get(ENGINE_HANDLE *handle, const void* cookie, item** item, const void* key, const int nkey, uint16_t vbucket){
	char strkey[nkey + 1];
	memcpy(strkey, key, nkey);
	strkey[nkey] = '\0';

	t_dummy_ng_item *it = dictionary_get(cache, strkey);

	if( it == NULL ){
		return ENGINE_NOT_STORED;
	}

	*item = it;
	return ENGINE_SUCCESS;
}

static ENGINE_ERROR_CODE dummy_ng_store(ENGINE_HANDLE *handle, const void *cookie, item* item, uint64_t *cas, ENGINE_STORE_OPERATION operation, uint16_t vbucket){
	t_dummy_ng_item *it = (t_dummy_ng_item*)item;

	char strkey[it->nkey + 1];
	memcpy(strkey, it->key, it->nkey);
	strkey[it->nkey] = '\0';

	t_dummy_ng_item* new_item = NULL;
	dummy_ng_allocate(handle, cookie, (void**)&new_item, it->key, it->nkey, it->ndata, it->flags, it->exptime);
	memcpy(new_item->data, it->data, it->ndata);

	dictionary_put(cache, strkey, new_item);

   *cas = 0;
   return ENGINE_SUCCESS;
}

static ENGINE_ERROR_CODE dummy_ng_flush(ENGINE_HANDLE* handle, const void* cookie, time_t when) {
	dictionary_clean(cache);

	return ENGINE_SUCCESS;
}

static const engine_info* dummy_ng_get_info(ENGINE_HANDLE* handle) {
	static engine_info info = {
	          .description = "Dummy Engine v0.1",
	          .num_features = 0,
	          .features = {
	               [0].feature = ENGINE_FEATURE_LRU,
	               [0].description = "No hay soporte de LRU"
	           }
	};

	return &info;
}

static ENGINE_ERROR_CODE dummy_ng_item_delete(ENGINE_HANDLE* handle, const void* cookie, const void* key, const size_t nkey, uint64_t cas, uint16_t vbucket) {
	return ENGINE_KEY_ENOENT;
}

static ENGINE_ERROR_CODE dummy_ng_get_stats(ENGINE_HANDLE* handle, const void* cookie, const char* stat_key, int nkey, ADD_STAT add_stat) {
	return ENGINE_SUCCESS;
}

static void dummy_ng_reset_stats(ENGINE_HANDLE* handle, const void *cookie) {

}

static ENGINE_ERROR_CODE dummy_ng_unknown_command(ENGINE_HANDLE* handle, const void* cookie, protocol_binary_request_header *request, ADD_RESPONSE response) {
	return ENGINE_ENOTSUP;
}

static void dummy_ng_item_set_cas(ENGINE_HANDLE *handle, const void *cookie, item* item, uint64_t val) {

}

void dummy_ng_dummp(int signal){

	void it(char *key, void *data){
		printf("%s\n", key);
	}

	dictionary_iterator(cache, it);
}
