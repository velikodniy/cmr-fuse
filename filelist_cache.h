#pragma once

#include "htable.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define PATH_SIZE 256

typedef struct {
  char basedir[PATH_SIZE];
  HTable *files;
  pthread_mutex_t *lock;
} filelist_cache_t;

enum file_kind_t {
    FK_FILE,
    FK_FOLDER
};

typedef struct {
    HTableNode node;
    char filename[PATH_SIZE];
    size_t filesize;
    enum file_kind_t kind;
    unsigned long mtime;
} filelist_cache_data_t;

filelist_cache_t *filelist_cache_create(void);
void filelist_cache_insert(filelist_cache_t *cache, filelist_cache_data_t *data);
filelist_cache_data_t *filelist_cache_find(filelist_cache_t *cache, const char *path);
void filelist_cache_clean(filelist_cache_t *cache);
void filelist_cache_free(filelist_cache_t *cache);

void filelist_cache_lock(filelist_cache_t *cache);
void filelist_cache_unlock(filelist_cache_t *cache);
