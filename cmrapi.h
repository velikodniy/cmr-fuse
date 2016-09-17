#pragma once

#include "list.h"
#include <curl/curl.h>
#include "filelist_cache.h"
#include "http_client.h"

#define TOKEN_SIZE (256)
#define URL_SIZE (256)

typedef struct cmr_t {
  char *user;
  char *domain;
  char *password;
  http_client_t *http;
  char token[TOKEN_SIZE];
  char download[URL_SIZE];
  char upload[URL_SIZE];
  filelist_cache_t *filelist_cache;
} cmr_t;

int cmr_init(cmr_t *cmr, const char *user, const char *domain, const char *password);
int cmr_login(cmr_t *cmr);
int cmr_sdc_cookies(cmr_t *cmr);
int cmr_get_token(cmr_t *cmr);
int cmr_get_shard_urls(cmr_t *cmr);

int cmr_list_dir(cmr_t *cmr, const char *dir, struct list_t **content);
size_t cmr_get_file(cmr_t *cmr, const char *filename, size_t size, off_t offset, char *buf);

void cmr_finalize(cmr_t *cmr);
