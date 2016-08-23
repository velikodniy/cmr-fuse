#include "cmrapi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>
#include "list.h"
#include "filelist_cache.h"
#include "http_client.h"
#include "utils.h"

int cmr_init(struct cmr_t *cmr, const char *user, const char *domain, const char *password) {
  memset(cmr, 0, sizeof(struct cmr_t));

  cmr->user = strdup(user);
  cmr->domain = strdup(domain);
  cmr->password = strdup(password);
  cmr->http = http_init();

  filelist_cache_create(&cmr->filelist_cache.files);

  return 0;
}

int cmr_login(struct cmr_t *cmr) {
  const char *request_string = "Login=%s&Domain=%s&Password=%s";
  char *request;
  asprintf(&request, request_string, cmr->user, cmr->domain, cmr->password);

  // do request for cookies
  http_request(cmr->http, HTTP_POST, "https://auth.mail.ru/cgi-bin/auth", NULL, 0, request, NULL);

  free(request);
  return 0;
}

int cmr_sdc_cookies(struct cmr_t *cmr) {
  return http_request(cmr->http, HTTP_GET, "https://auth.mail.ru/sdc?from=https://cloud.mail.ru/home", NULL, 1, NULL, NULL);
}

int cmr_get_token(struct cmr_t *cmr) {
  struct curl_slist *headers = NULL;
  buffer_t buffer;
  buffer_init(&buffer);

  headers = curl_slist_append(headers, "Accept: application/json");

  http_request(cmr->http, HTTP_GET, "https://cloud.mail.ru/api/v2/tokens/csrf", headers, 0, NULL, &buffer);

  json_t *root, *status, *body, *token;
  json_error_t error;
  root = json_loads(buffer.data, 0, &error);
  
  status = json_object_get(root, "status");
  if (json_integer_value(status) == 200) {
    body = json_object_get(root, "body");
    token = json_object_get(body, "token");
    strncpy(cmr->token, json_string_value(token), sizeof(cmr->token));
  } else {
    fprintf(stderr, "cmr_get_token() failed: JSON\n");
    return 1;
  }
  
  curl_slist_free_all(headers);
  json_decref(root); 
  buffer_free(&buffer);
  return 0;
}

int cmr_get_shard_urls(struct cmr_t *cmr) {
  buffer_t buffer;
  buffer_init(&buffer);
  
  char *url;
  asprintf(&url, "https://cloud.mail.ru/api/v2/dispatcher?token=%s", cmr->token);

  http_request(cmr->http, HTTP_GET, url, NULL, 1, NULL, &buffer);

  json_t *root, *status;
  json_error_t error;
  root = json_loads(buffer.data, 0, &error);
  status = json_object_get(root, "status");
  if (json_integer_value(status) == 200) {
    json_t *body, *get, *upload, *first, *url;

    body = json_object_get(root, "body");
    get = json_object_get(body, "get");
    first = json_array_get(get, 0);
    url = json_object_get(first, "url");
    strncpy(cmr->download, json_string_value(url), 128);

    upload = json_object_get(body, "upload");
    first = json_array_get(upload, 0);
    url = json_object_get(first, "url");
    strncpy(cmr->upload, json_string_value(url), 128);
  }
  
  json_decref(root); 
  buffer_free(&buffer);
  free(url);
  
  return 0;
}

int cmr_list_dir(struct cmr_t *cmr, const char *dir, struct list_t **content) {
  buffer_t buffer;
  buffer_init(&buffer);

  char *encoded_dir = curl_easy_escape(cmr->http, dir, 0);
  
  char *dir_url;
  asprintf(&dir_url, "https://cloud.mail.ru/api/v2/folder?token=%s&home=%s", cmr->token, encoded_dir);

  http_request(cmr->http, HTTP_GET, dir_url, NULL, 1, NULL, &buffer);

  list_init(content);

  filelist_cache_free(&(cmr->filelist_cache.files));
  strncpy(cmr->filelist_cache.basedir, dir, sizeof(cmr->filelist_cache.basedir));

  json_t *root, *status;
  json_error_t error;
  root = json_loads(buffer.data, 0, &error);
  status = json_object_get(root, "status");
  if (json_integer_value(status) == 200) {
    json_t *body, *list, *item, *name, *kind, *size, *mtime, *home;

    body = json_object_get(root, "body");
    list = json_object_get(body, "list");

    size_t array_size = json_array_size(list);
    for(size_t i = 0; i < array_size; i++) {
      filelist_cache_data_t data;

      item = json_array_get(list, i);
      name = json_object_get(item, "name");
      char *nm = strdup(json_string_value(name));
      list_append(content, nm);

      home = json_object_get(item, "home");
      strncpy(data.filename, json_string_value(home), sizeof(data.filename));

      kind = json_object_get(item, "kind");
      if (strcmp(json_string_value(kind), "folder") == 0)
        data.kind = FK_FOLDER;
      else
        data.kind = FK_FILE;

      size = json_object_get(item, "size");
      data.filesize = json_integer_value(size);

      if (data.kind == FK_FILE) {
        mtime = json_object_get(item, "mtime");
        data.mtime = json_integer_value(mtime);
      } else
        data.mtime = 0;

      filelist_cache_insert(&cmr->filelist_cache.files, &data);
    }
  }

  json_decref(root); 
  free(dir_url);
  curl_free(encoded_dir);
  buffer_free(&buffer);
  return 0;
}

size_t cmr_get_file(struct cmr_t *cmr, const char *filename, size_t size, off_t offset, char *buf) {
  if (*filename == '/')
    filename++;
  
  buffer_t buffer;
  buffer_init(&buffer);

  char *encoded_filename;
  char *download_url, *range_header;
  
  encoded_filename = curl_easy_escape(cmr->http, filename, 0);

  asprintf(&download_url, "%s%s", cmr->download, encoded_filename);
  asprintf(&range_header, "Range: bytes=%ld-%ld", offset, offset+size-1);

  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, range_header);

  http_request(cmr->http, HTTP_GET, download_url, headers, 1, NULL, &buffer);

  curl_easy_setopt(cmr->http, CURLOPT_HTTPHEADER, headers);

  memcpy(buf, buffer.data, buffer.length);

  size_t len = buffer.length;
  
  curl_slist_free_all(headers);
  curl_free(encoded_filename);
  free(range_header);
  free(download_url);
  buffer_free(&buffer);
  return len;
}

void cmr_finalize(struct cmr_t *cmr) {
  free(cmr->user);
  free(cmr->domain);
  free(cmr->password);
  http_free(cmr->http);
  filelist_cache_free(&(cmr->filelist_cache.files));
}
