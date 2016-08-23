#pragma once

#include "list.h"
#include <curl/curl.h>
#include "filelist_cache.h"
#include "http_client.h"

#define TOKEN_SIZE (256)
#define URL_SIZE (256)

struct cmr_t {
  char *user;
  char *domain;
  char *password;
  http_client_t *http;
  char token[TOKEN_SIZE];
  char download[URL_SIZE];
  char upload[URL_SIZE];
  filelist_cache_t filelist_cache;
};

int cmr_init(struct cmr_t *cmr, const char *user, const char *domain, const char *password);
int cmr_login(struct cmr_t *cmr);
int cmr_sdc_cookies(struct cmr_t *cmr);
int cmr_get_token(struct cmr_t *cmr);
int cmr_get_shard_urls(struct cmr_t *cmr);

int cmr_list_dir(struct cmr_t *cmr, const char *dir, struct list_t **content);
size_t cmr_get_file(struct cmr_t *cmr, const char *filename, size_t size, off_t offset, char *buf);

void cmr_finalize(struct cmr_t *cmr);
