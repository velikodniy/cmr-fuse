#include "filelist_cache.h"

bool keyeq_func(const HTableNode* a_, const HTableNode* b_, void *arg)
{
  (void) arg;
  filelist_cache_data_t* a = (filelist_cache_data_t*)a_;
  filelist_cache_data_t* b = (filelist_cache_data_t*)b_;
  return (strcmp(a->filename, b->filename) == 0);
}

uint32_t hash_func(const HTableNode* a_, void *arg)
{
  (void) arg;
  filelist_cache_data_t* a = (filelist_cache_data_t*)a_;
  return htable_default_hash(a->filename, strlen(a->filename));
}

void* alloc_func(size_t size, void *arg)
{
  (void) arg;
  return malloc(size);
}

void free_func(void* mem, void *arg)
{
  (void) arg;
  free(mem);
}

filelist_cache_t *filelist_cache_create(void) {
  filelist_cache_t *cache = malloc(sizeof(filelist_cache_t));
  cache->files = malloc(sizeof(HTable));
  cache->lock = malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(cache->lock, NULL);
  htable_create(cache->files,
		sizeof(filelist_cache_data_t),
		hash_func,
		keyeq_func,
		alloc_func,
		free_func,
		NULL,
		NULL);
  cache->basedir[0] = 0;
  return cache;
}

void filelist_cache_insert(filelist_cache_t *cache, filelist_cache_data_t *data) {
  bool new_item;
  htable_insert(cache->files, (HTableNode*)data, &new_item);
}

filelist_cache_data_t *filelist_cache_find(filelist_cache_t *cache, const char *path) {
  filelist_cache_data_t query;
  strncpy(query.filename, path, sizeof(query.filename));
  return (filelist_cache_data_t*)htable_find(cache->files, (HTableNode*)&query);
}

void filelist_cache_clean(filelist_cache_t *cache) {
  htable_free_items(cache->files);
  htable_create(cache->files,
                sizeof(filelist_cache_data_t),
                hash_func,
                keyeq_func,
                alloc_func,
                free_func,
                NULL,
                NULL);
  cache->basedir[0] = 0;
}

void filelist_cache_free(filelist_cache_t *cache) {
  htable_free_items(cache->files);
  pthread_mutex_destroy(cache->lock);
  free(cache->lock);
  free(cache->files);
  free(cache);
}

void filelist_cache_lock(filelist_cache_t *cache) {
  pthread_mutex_lock(cache->lock);
}

void filelist_cache_unlock(filelist_cache_t *cache) {
  pthread_mutex_unlock(cache->lock);
}