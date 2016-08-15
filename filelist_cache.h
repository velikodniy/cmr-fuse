#pragma once

#include "htable.h"
#include <stdlib.h>
#include <string.h>

#define PATH_SIZE 256

typedef struct {
  char basedir[PATH_SIZE];
  HTable files;
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

void filelist_cache_create(HTable *hTable);
void filelist_cache_insert(HTable *hTable, filelist_cache_data_t *data);
void filelist_cache_free(HTable *hTable);
