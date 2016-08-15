#include <curl/curl.h>
#include "curl_wrapper.h"

CURL* curl_init(int verbose){
  CURL* curl;
  curl_global_init(CURL_GLOBAL_DEFAULT);
  if((curl = curl_easy_init()) == NULL){
    fprintf(stderr, "curl_init() failed\n");
    return NULL;
  }

  curl_easy_setopt(curl, CURLOPT_USERAGENT, "PyMailCloud/(0.2)");
  curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "");

  if(verbose == 1)
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

  return curl;
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

int curl_request(CURL *curl,
                 enum http_method method,
                 char *url,
                 struct curl_slist *headers,
                 int follow_location,
                 char *request,
                 struct buffer_t *buffer){
  CURLcode res;

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
  if(follow_location == NULL) follow_location = 0;
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, follow_location);
  if(method == HTTP_POST)
    curl_easy_setopt(curl, CURLOPT_POST, 1);
  else
    curl_easy_setopt(curl, CURLOPT_POST, 0);
  curl_easy_setopt(curl, CURLOPT_URL, url);
  if(request != NULL)
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request);
  else
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, NULL);
  res = curl_easy_perform(curl);

  if(request != NULL && request != NULL)
    free(request);

  if(res != CURLE_OK) {
    fprintf(stderr, "curl failed: %s\n", curl_easy_strerror(res));
    return 1;
  }

  return 0;
}
