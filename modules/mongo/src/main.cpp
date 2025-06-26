#include "lib/base/base.h"
#include "KabaExporter.h"
#include <stdio.h>
#include <bson/bson.h>
#include <mongoc/mongoc.h>


extern "C" {

__attribute__ ((visibility ("default")))
void export_symbols(kaba::Exporter* e) {
	//printf("<mongo export>\n");
	e->link("_mongoc_init", (void*)&mongoc_init);
	e->link("_mongoc_cleanup", (void*)&mongoc_cleanup);
	e->link("_mongoc_client_new", (void*)&mongoc_client_new);
	e->link("_mongoc_client_destroy", (void*)&mongoc_client_destroy);
	e->link("_mongoc_client_get_collection", (void*)&mongoc_client_get_collection);
	e->link("_mongoc_collection_find_with_opts", (void*)&mongoc_collection_find_with_opts);
	e->link("_mongoc_collection_aggregate", (void*)&mongoc_collection_aggregate);
	e->link("_mongoc_collection_insert_one", (void*)&mongoc_collection_insert_one);
	e->link("_mongoc_collection_replace_one", (void*)&mongoc_collection_replace_one);
	e->link("_mongoc_collection_delete_one", (void*)&mongoc_collection_delete_one);
	e->link("_mongoc_collection_delete_many", (void*)&mongoc_collection_delete_many);
	e->link("_mongoc_cursor_next", (void*)&mongoc_cursor_next);
	e->link("_mongoc_cursor_destroy", (void*)&mongoc_cursor_destroy);
	e->link("_mongoc_collection_destroy", (void*)&mongoc_collection_destroy);
	
	e->link("_bson_new_from_json", (void*)&bson_new_from_json);
	e->link("_bson_as_canonical_extended_json", (void*)&bson_as_canonical_extended_json);
	e->link("_bson_as_relaxed_extended_json", (void*)&bson_as_relaxed_extended_json);
	e->link("_bson_free", (void*)&bson_free);
	e->link("_bson_destroy", (void*)&bson_destroy);
	e->link("_bson_new_from_data", (void*)&bson_new_from_data);
	e->link("_bson_iter_init", (void*)&bson_iter_init);
	e->link("_bson_iter_next", (void*)&bson_iter_next);
	e->link("_bson_iter_key", (void*)&bson_iter_key);
	e->link("_bson_iter_int32", (void*)&bson_iter_int32);
	e->link("_bson_iter_int64", (void*)&bson_iter_int64);
	e->link("_bson_iter_double", (void*)&bson_iter_double);
	e->link("_bson_iter_utf8", (void*)&bson_iter_utf8);
	e->link("_bson_iter_array", (void*)&bson_iter_array);
	e->link("_bson_iter_oid", (void*)&bson_iter_oid);
	e->link("_bson_oid_to_string", (void*)&bson_oid_to_string);
	e->link("_bson_iter_type", (void*)&bson_iter_type);
}
}


