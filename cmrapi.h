#pragma once

#include <curl/curl.h>

#define TOKEN_SIZE (256)

struct cmr_t {
  char *user;
  char *domain;
  char *password;
  CURL *curl;
  char token[TOKEN_SIZE];
};

struct buffer_t {
  char* data; 
  size_t length;
  size_t size;
};

void buffer_init(struct buffer_t *buffer);
void buffer_reset(struct buffer_t *buffer);
void buffer_free(struct buffer_t *buffer);

int cmr_init(struct cmr_t *cmr, const char *user, const char *domain, const char *password);
void cmr_response_ignore(struct cmr_t *cmr);
void cmr_response_to_buffer(struct cmr_t *cmr, struct buffer_t *buffer);
int cmr_login(struct cmr_t *cmr);
int cmr_sdc_cookies(struct cmr_t *cmr);
int cmr_get_token(struct cmr_t *cmr);
int cmr_get_shard(struct cmr_t *cmr);

//int cmr_list_dir(struct cmr_t *cmr, list_t *content);
int cmr_get_file(struct cmr_t *cmr, char *filename, size_t size, off_t offset, char *buf);

void cmr_finalize(struct cmr_t *cmr);
