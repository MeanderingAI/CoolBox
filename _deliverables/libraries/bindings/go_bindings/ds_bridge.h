#ifndef COOLBOX_DS_BRIDGE_H
#define COOLBOX_DS_BRIDGE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void* coolbox_ds_list_new(void);
void coolbox_ds_list_free(void* handle);
int coolbox_ds_list_push_front(void* handle, int64_t value);
int coolbox_ds_list_push_back(void* handle, int64_t value);
int coolbox_ds_list_insert(void* handle, size_t index, int64_t value);
int coolbox_ds_list_pop_front(void* handle, int* removed);
int coolbox_ds_list_pop_back(void* handle, int* removed);
int coolbox_ds_list_remove_at(void* handle, size_t index, int* removed);
int coolbox_ds_list_remove_value(void* handle, int64_t value, int* removed);
int coolbox_ds_list_at(const void* handle, size_t index, int64_t* value);
int coolbox_ds_list_front(const void* handle, int64_t* value);
int coolbox_ds_list_back(const void* handle, int64_t* value);
int coolbox_ds_list_find(const void* handle, int64_t value);
size_t coolbox_ds_list_size(const void* handle);
int coolbox_ds_list_empty(const void* handle);
int coolbox_ds_list_clear(void* handle);
int coolbox_ds_list_reverse(void* handle);
size_t coolbox_ds_list_values(const void* handle, int64_t* values, size_t capacity);

void* coolbox_ds_heap_new(int min_heap);
void coolbox_ds_heap_free(void* handle);
int coolbox_ds_heap_push(void* handle, int64_t value);
int coolbox_ds_heap_pop(void* handle);
int coolbox_ds_heap_top(const void* handle, int64_t* value);
size_t coolbox_ds_heap_size(const void* handle);
int coolbox_ds_heap_empty(const void* handle);
int coolbox_ds_heap_clear(void* handle);

void* coolbox_ds_indexed_queue_new(void);
void coolbox_ds_indexed_queue_free(void* handle);
int coolbox_ds_indexed_queue_push(void* handle, int64_t key, double priority);
int coolbox_ds_indexed_queue_update(void* handle, int64_t key, double priority);
int coolbox_ds_indexed_queue_pop(void* handle);
int coolbox_ds_indexed_queue_top(const void* handle, int64_t* key, double* priority);
int coolbox_ds_indexed_queue_contains(const void* handle, int64_t key);
size_t coolbox_ds_indexed_queue_size(const void* handle);
int coolbox_ds_indexed_queue_empty(const void* handle);
int coolbox_ds_indexed_queue_clear(void* handle);

void* coolbox_ds_map_new(int concurrent, size_t initial_capacity);
void coolbox_ds_map_free(void* handle);
int coolbox_ds_map_insert(void* handle, int64_t key, int64_t value);
int coolbox_ds_map_remove(void* handle, int64_t key, int* removed);
int coolbox_ds_map_get(const void* handle, int64_t key, int64_t* value, int* found);
int coolbox_ds_map_contains(const void* handle, int64_t key);
size_t coolbox_ds_map_size(const void* handle);
int coolbox_ds_map_clear(void* handle);

void* coolbox_ds_tree_new(int kind);
void coolbox_ds_tree_free(void* handle);
int coolbox_ds_tree_insert(void* handle, int64_t value);
int coolbox_ds_tree_remove(void* handle, int64_t value, int* removed);
int coolbox_ds_tree_contains(void* handle, int64_t value);
size_t coolbox_ds_tree_size(const void* handle);
int coolbox_ds_tree_empty(const void* handle);
int coolbox_ds_tree_clear(void* handle);
int coolbox_ds_tree_min(const void* handle, int64_t* value);
int coolbox_ds_tree_max(const void* handle, int64_t* value);
size_t coolbox_ds_tree_values(const void* handle, int64_t* values, size_t capacity);

void* coolbox_ds_filter_new(int kind, size_t expected_insertions, double false_positive_rate);
void coolbox_ds_filter_free(void* handle);
int coolbox_ds_filter_insert(void* handle, int64_t value);
int coolbox_ds_filter_contains(const void* handle, int64_t value);
size_t coolbox_ds_filter_insert_count(const void* handle);
size_t coolbox_ds_filter_approximate_count(const void* handle);
double coolbox_ds_filter_estimated_rate(const void* handle);
int coolbox_ds_filter_clear(void* handle);

void* coolbox_ds_veb_new(uint64_t universe_size);
void coolbox_ds_veb_free(void* handle);
int coolbox_ds_veb_insert(void* handle, uint64_t value);
int coolbox_ds_veb_remove(void* handle, uint64_t value);
int coolbox_ds_veb_contains(const void* handle, uint64_t value);
uint64_t coolbox_ds_veb_min(const void* handle);
uint64_t coolbox_ds_veb_max(const void* handle);
uint64_t coolbox_ds_veb_successor(const void* handle, uint64_t value);
uint64_t coolbox_ds_veb_predecessor(const void* handle, uint64_t value);
uint64_t coolbox_ds_veb_universe_size(const void* handle);
int coolbox_ds_veb_empty(const void* handle);

void* coolbox_ds_array_new(size_t rank, const size_t* shape);
void coolbox_ds_array_free(void* handle);
size_t coolbox_ds_array_rank(const void* handle);
size_t coolbox_ds_array_size(const void* handle);
int coolbox_ds_array_shape(const void* handle, size_t* shape, size_t capacity);
int coolbox_ds_array_get(const void* handle, const size_t* indices, size_t rank, double* value);
int coolbox_ds_array_set(void* handle, const size_t* indices, size_t rank, double value);

void* coolbox_ds_queue_new(void);
void coolbox_ds_queue_free(void* handle);
int coolbox_ds_queue_enqueue(void* handle, int64_t value);
int coolbox_ds_queue_enqueue_batch(void* handle, const int64_t* values, size_t count);
int coolbox_ds_queue_try_dequeue(void* handle, int64_t* value, int* found);
size_t coolbox_ds_queue_size(const void* handle);
int coolbox_ds_queue_empty(const void* handle);

void* coolbox_ds_concurrent_list_new(void);
void coolbox_ds_concurrent_list_free(void* handle);
int coolbox_ds_concurrent_list_push_front(void* handle, int64_t value);
int coolbox_ds_concurrent_list_push_back(void* handle, int64_t value);
int coolbox_ds_concurrent_list_pop_front(void* handle, int64_t* value, int* found);
int coolbox_ds_concurrent_list_remove(void* handle, int64_t value, int* removed);
int coolbox_ds_concurrent_list_contains(const void* handle, int64_t value);
size_t coolbox_ds_concurrent_list_size(const void* handle);
int coolbox_ds_concurrent_list_empty(const void* handle);

void* coolbox_ds_bplus_tree_new(void);
void coolbox_ds_bplus_tree_free(void* handle);
int coolbox_ds_bplus_tree_insert(void* handle, int64_t key, int64_t value);
int coolbox_ds_bplus_tree_get(const void* handle, int64_t key, int64_t* value, int* found);
int coolbox_ds_bplus_tree_remove(void* handle, int64_t key, int* removed);
size_t coolbox_ds_bplus_tree_size(const void* handle);
int coolbox_ds_bplus_tree_clear(void* handle);
int coolbox_ds_bplus_tree_entries(const void* handle, int range, int64_t minimum, int64_t maximum,
                                  size_t* count, int64_t* keys, int64_t* values, size_t capacity);

void* coolbox_ds_radix_tree_new(void);
void coolbox_ds_radix_tree_free(void* handle);
int coolbox_ds_radix_tree_insert(void* handle, const char* key, int64_t value);
int coolbox_ds_radix_tree_get(const void* handle, const char* key, int64_t* value, int* found);
int coolbox_ds_radix_tree_remove(void* handle, const char* key, int* removed);

void* coolbox_ds_doubly_list_new(void);
void coolbox_ds_doubly_list_free(void* handle);
int coolbox_ds_doubly_list_push_front(void* handle, int64_t value);
int coolbox_ds_doubly_list_push_back(void* handle, int64_t value);
int coolbox_ds_doubly_list_insert(void* handle, size_t index, int64_t value);
int coolbox_ds_doubly_list_pop_front(void* handle, int* removed);
int coolbox_ds_doubly_list_pop_back(void* handle, int* removed);
int coolbox_ds_doubly_list_remove_at(void* handle, size_t index, int* removed);
int coolbox_ds_doubly_list_front(const void* handle, int64_t* value);
int coolbox_ds_doubly_list_back(const void* handle, int64_t* value);
int coolbox_ds_doubly_list_at(const void* handle, size_t index, int64_t* value);
size_t coolbox_ds_doubly_list_size(const void* handle);
int coolbox_ds_doubly_list_empty(const void* handle);
int coolbox_ds_doubly_list_clear(void* handle);

void* coolbox_ds_legacy_heap_new(int min_heap);
void coolbox_ds_legacy_heap_free(void* handle);
int coolbox_ds_legacy_heap_push(void* handle, int64_t value);
int coolbox_ds_legacy_heap_pop(void* handle);
int coolbox_ds_legacy_heap_top(const void* handle, int64_t* value);
size_t coolbox_ds_legacy_heap_size(const void* handle);
int coolbox_ds_legacy_heap_empty(const void* handle);
int coolbox_ds_legacy_heap_clear(void* handle);

void* coolbox_ds_lsm_new(size_t threshold);
void coolbox_ds_lsm_free(void* handle);
int coolbox_ds_lsm_put(void* handle, int64_t key, int64_t value);
int coolbox_ds_lsm_get(const void* handle, int64_t key, int64_t* value, int* found);
int coolbox_ds_lsm_remove(void* handle, int64_t key, int* removed);
int coolbox_ds_lsm_contains(const void* handle, int64_t key);
int coolbox_ds_lsm_flush(void* handle);
int coolbox_ds_lsm_compact(void* handle);
int coolbox_ds_lsm_clear(void* handle);
size_t coolbox_ds_lsm_size(const void* handle);
size_t coolbox_ds_lsm_memtable_size(const void* handle);
size_t coolbox_ds_lsm_sstable_count(const void* handle);

void* coolbox_ds_maple_new(void);
void coolbox_ds_maple_free(void* handle);
int coolbox_ds_maple_insert(void* handle, int64_t key, int64_t value);
int coolbox_ds_maple_get(const void* handle, int64_t key, int64_t* value, int* found);
int coolbox_ds_maple_remove(void* handle, int64_t key, int* removed);

void* coolbox_ds_cover_tree_new(int faster, size_t dimensions);
void coolbox_ds_cover_tree_free(void* handle);
int coolbox_ds_cover_tree_build(void* handle, const double* values, size_t count, size_t dimensions);
int coolbox_ds_cover_tree_insert(void* handle, const double* item, size_t dimensions);
int coolbox_ds_cover_tree_clear(void* handle);
size_t coolbox_ds_cover_tree_size(const void* handle);
int coolbox_ds_cover_tree_validate(const void* handle);
int coolbox_ds_cover_tree_query(const void* handle, const double* query, size_t dimensions,
                                size_t k, double radius, int use_radius, size_t* count,
                                size_t* indices, double* distances, double* items, size_t capacity);

#ifdef __cplusplus
}
#endif

#endif
