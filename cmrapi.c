#include "cmrapi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <jansson.h>

#define RESPONSE_BUFFER_SIZE (1024*1024)

void buffer_init(struct buffer_t *buffer) {
  buffer->data = malloc(RESPONSE_BUFFER_SIZE);
  if (buffer->data == NULL) exit(1);
  buffer->length = 0;
  buffer->size = RESPONSE_BUFFER_SIZE;
}

void buffer_reset(struct buffer_t *buffer) {
  buffer->length = 0;
}

void buffer_free(struct buffer_t *buffer) {
  free(buffer->data);
  buffer->data = NULL;
  buffer->length = 0;
}

size_t write_to_null(void *curl_buffer, size_t size, size_t nmemb, void *userptr)
{
  return size * nmemb;
}

size_t write_to_buffer(void *curl_buffer, size_t size, size_t nmemb, void *userptr) {
  struct buffer_t *buffer = userptr;
  size_t real_size = size * nmemb;

  while (buffer->length + real_size >= buffer->size) {
    size_t new_buffer_size = (size_t)(buffer->size * 1.618);
    buffer->data = realloc(buffer->data, new_buffer_size);
    if (buffer->data == NULL) exit(1);
    buffer->size = new_buffer_size;
  }

  memcpy(buffer->data + buffer->length, curl_buffer, real_size);
  buffer->length += real_size;
  buffer->data[buffer->length] = 0;
  return real_size;
}

int cmr_init(struct cmr_t *cmr,
	     const char *user, const char *domain, const char *password) {
  memset(cmr, 0, sizeof(struct cmr_t));
  curl_global_init(CURL_GLOBAL_DEFAULT);
  if((cmr->curl = curl_easy_init()) == NULL){
    fprintf(stderr, "cmr_init() failed\n");
    return 1;
  }

  curl_easy_setopt(cmr->curl, CURLOPT_USERAGENT, "PyMailCloud/(0.2)");
  curl_easy_setopt(cmr->curl, CURLOPT_COOKIEFILE, "");
  //  curl_easy_setopt(cmr->curl, CURLOPT_VERBOSE, 1);
  cmr_response_ignore(cmr);

  size_t
    luser = 1 + strlen(user),
    ldomain = 1 + strlen(domain),
    lpassword = 1 + strlen(password);
  
  cmr->user = malloc(luser);
  cmr->domain = malloc(ldomain);
  cmr->password = malloc(lpassword);

  if ((cmr->user == NULL) ||
      (cmr->domain == NULL) ||
      (cmr->password == NULL)) exit(1);

  strncpy(cmr->user, user, luser);
  strncpy(cmr->domain, domain, ldomain);
  strncpy(cmr->password, password, lpassword);
    
  return 0;
}

void cmr_response_ignore(struct cmr_t *cmr) {
  curl_easy_setopt(cmr->curl, CURLOPT_WRITEFUNCTION, write_to_null);
}

void cmr_response_to_buffer(struct cmr_t *cmr, struct buffer_t *buffer) {
  curl_easy_setopt(cmr->curl, CURLOPT_WRITEFUNCTION, write_to_buffer);
  curl_easy_setopt(cmr->curl, CURLOPT_WRITEDATA, buffer);
}

int cmr_login(struct cmr_t *cmr) {
  CURLcode res;
  const char *request_string = "Login=%s&Domain=%s&Password=%s";
  size_t request_size = 1 + strlen(request_string)
    + strlen(cmr->user) + strlen(cmr->domain) + strlen(cmr->password);
  char *request = malloc(request_size);
  
  snprintf(request, request_size, request_string, cmr->user, cmr->domain, cmr->password);

  curl_easy_setopt(cmr->curl, CURLOPT_FOLLOWLOCATION, 0);
  curl_easy_setopt(cmr->curl, CURLOPT_POST, 1);
  curl_easy_setopt(cmr->curl, CURLOPT_URL, "https://auth.mail.ru/cgi-bin/auth");
  curl_easy_setopt(cmr->curl, CURLOPT_POSTFIELDS, request);
  res = curl_easy_perform(cmr->curl);
  if(res != CURLE_OK) {
    fprintf(stderr, "cmr_login() failed: %s\n", curl_easy_strerror(res));
    return 1;
  }
  curl_easy_setopt(cmr->curl, CURLOPT_POST, 0);
  curl_easy_setopt(cmr->curl, CURLOPT_FOLLOWLOCATION, 1);

  free(request);
  return 0;
}

int cmr_sdc_cookies(struct cmr_t *cmr) {
  CURLcode res;

  curl_easy_setopt(cmr->curl, CURLOPT_URL, "https://auth.mail.ru/sdc?from=https://cloud.mail.ru/home");
  res = curl_easy_perform(cmr->curl);
  if(res != CURLE_OK) {
    fprintf(stderr, "cmr_sdc_cookies() failed: %s\n", curl_easy_strerror(res));
    return 1;
  }
  return 0;
}

int cmr_get_token(struct cmr_t *cmr) {
  CURLcode res;
  struct curl_slist *headers = NULL;
  struct buffer_t buffer;
  buffer_init(&buffer);

  headers = curl_slist_append(headers, "Accept: application/json");
  curl_easy_setopt(cmr->curl, CURLOPT_HTTPHEADER, headers);

  cmr_response_to_buffer(cmr, &buffer);
  curl_easy_setopt(cmr->curl, CURLOPT_POST, 1);
  curl_easy_setopt(cmr->curl, CURLOPT_URL, "https://cloud.mail.ru/api/v2/tokens/csrf");
  res = curl_easy_perform(cmr->curl);
  if(res != CURLE_OK) {
    fprintf(stderr, "cmr_get_token() failed: %s\n", curl_easy_strerror(res));
    return 1;
  }
  curl_easy_setopt(cmr->curl, CURLOPT_POST, 0); 
  curl_easy_setopt(cmr->curl, CURLOPT_HTTPHEADER, NULL);
  cmr_response_ignore(cmr);

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

  CURLcode res;

  struct buffer_t buffer;
  buffer_init(&buffer);
  
  const char *url_string = "https://cloud.mail.ru/api/v2/dispatcher?token=%s";
  size_t url_size = 1 + strlen(url_string) + strlen(cmr->token);
  char *url = malloc(url_size);
  
  snprintf(url, url_size, url_string, cmr->token);

  cmr_response_to_buffer(cmr, &buffer);
  curl_easy_setopt(cmr->curl, CURLOPT_URL, url);

  res = curl_easy_perform(cmr->curl);
  if(res != CURLE_OK) {
    fprintf(stderr, "cmr_get_shard_urls() failed: %s\n", curl_easy_strerror(res));
    return 1;
  }
  cmr_response_ignore(cmr);

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

//int cmr_list_dir(struct cmr_t *cmr, list_t *content);


size_t cmr_get_file(struct cmr_t *cmr, char *filename, size_t size, off_t offset, char *buf) {
  CURLcode res;

  if (*filename == '/')
    filename++;
  
  struct buffer_t buffer;
  buffer_init(&buffer);

  char *encoded_filename;
  char *download_url, *range_header;
  
  encoded_filename = curl_easy_escape(cmr->curl, filename, 0);

  size_t du_size = snprintf(NULL, 0, "%s%s", cmr->download, encoded_filename);
  download_url = malloc(1 + du_size);
  snprintf(download_url, du_size+1, "%s%s", cmr->download, encoded_filename);

  size_t rh_size = snprintf(NULL, 0, "Range: bytes=%ld-%ld", offset, offset+size-1);
  range_header = malloc(1 + rh_size);
  snprintf(range_header, rh_size+1, "Range: bytes=%ld-%ld", offset, offset+size-1);

  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, range_header);
  curl_easy_setopt(cmr->curl, CURLOPT_HTTPHEADER, headers);

  cmr_response_to_buffer(cmr, &buffer);
  curl_easy_setopt(cmr->curl, CURLOPT_URL, download_url);
  res = curl_easy_perform(cmr->curl);
  if(res != CURLE_OK) {
    fprintf(stderr, "cmr_get_file() failed: %s\n", curl_easy_strerror(res));
    return 0;
  }
  curl_easy_setopt(cmr->curl, CURLOPT_HTTPHEADER, NULL);
  cmr_response_ignore(cmr);

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
  curl_easy_cleanup(cmr->curl);
  curl_global_cleanup();
}
