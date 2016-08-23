#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "http_client.h"

#define RESPONSE_BUFFER_SIZE (1024*1024)

void buffer_init(struct buffer_t *buffer) {
  buffer->data = malloc(RESPONSE_BUFFER_SIZE);
  if (buffer->data == NULL) exit(1);
  buffer->length = 0;
  buffer->size = RESPONSE_BUFFER_SIZE;
}

void buffer_free(struct buffer_t *buffer) {
  free(buffer->data);
  buffer->data = NULL;
  buffer->length = 0;
}

void curl_lock(CURL *curl, curl_lock_data data, curl_lock_access access, void *userptr) {
  (void)curl;
  (void)data;
  (void)access;
  //TODO: use separate locks for different kinds of data
  pthread_mutex_lock((pthread_mutex_t*)userptr);
}

void curl_unlock(CURL *curl, curl_lock_data data, void *userptr) {
  (void)curl;
  (void)data;
  pthread_mutex_unlock((pthread_mutex_t*)userptr);
}

http_client_t *http_init(void){
  http_client_t *hc = malloc(sizeof(http_client_t));
  curl_global_init(CURL_GLOBAL_DEFAULT);
  if((hc->curl_share = curl_share_init()) == NULL){
    fprintf(stderr, "curl_share_init() failed\n");
    free(hc);
    return NULL;
  }

  hc->curl_mutex = malloc(sizeof(pthread_mutex_t));

  if (0 != pthread_mutex_init(hc->curl_mutex, NULL))
    return NULL;

  curl_share_setopt(hc->curl_share, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);
  curl_share_setopt(hc->curl_share, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
  curl_share_setopt(hc->curl_share, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);
  curl_share_setopt(hc->curl_share, CURLSHOPT_USERDATA, hc->curl_mutex);
  curl_share_setopt(hc->curl_share, CURLSHOPT_LOCKFUNC, curl_lock);
  curl_share_setopt(hc->curl_share, CURLSHOPT_UNLOCKFUNC, curl_unlock);

  hc->verbose = 0;
  return hc;
}

size_t write_to_null(void *curl_buffer, size_t size, size_t nmemb, void *userptr)
{
  (void) curl_buffer;
  (void) userptr;
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

int http_request(http_client_t *hc,
                 enum http_method method,
                 char *url,
                 struct curl_slist *headers,
                 int follow_location,
                 char *request,
                 struct buffer_t *buffer){
  CURL *curl = curl_easy_init();
  CURLcode res;

  curl_easy_setopt(curl, CURLOPT_SHARE, hc->curl_share);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "PyMailCloud/(0.2)");
  curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "");

  if(hc->verbose == 1)
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

  if(headers != NULL)
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  else
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, NULL);

  if(buffer != NULL){
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_buffer);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, buffer);
  }else{
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_null);
  }

  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, follow_location);
  if(method == HTTP_POST) {
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    if(request != NULL)
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request);
    else
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, NULL);
  }
  else
    curl_easy_setopt(curl, CURLOPT_POST, 0);
  curl_easy_setopt(curl, CURLOPT_URL, url);
  res = curl_easy_perform(curl);

  if(res != CURLE_OK) {
    fprintf(stderr, "curl failed: %s\n", curl_easy_strerror(res));
    curl_easy_cleanup(curl);
    return 1;
  }

  curl_easy_cleanup(curl);
  return 0;
}

void http_free(http_client_t *hc) {
  // FIXME: Crashes with core dump (double free on umount). Does libfuse set exit handler?
  //curl_share_cleanup(hc->curl_share);
  //curl_global_cleanup();
  pthread_mutex_destroy(hc->curl_mutex);
  free(hc->curl_mutex);
  free(hc);
}
