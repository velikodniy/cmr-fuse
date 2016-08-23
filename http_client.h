#pragma once

#include <curl/curl.h>
#include <pthread.h>

typedef struct http_client_t {
  CURLSH *curl_share;
  pthread_mutex_t *curl_mutex;
  int verbose;
} http_client_t;

typedef struct buffer_t {
  char* data;
  size_t length;
  size_t size;
} buffer_t;

void buffer_init(buffer_t *buffer);
void buffer_free(buffer_t *buffer);

enum http_method{
  HTTP_GET,
  HTTP_POST
};

/**
 * Initialize CURL
 * @param verbose 1 - verbose, 0 - silent mode
 * @return HTTP client handler
 */
http_client_t *http_init(void);

/**
 * Make HTTP POST request
 * @param hc HTTP client handler
 * @param method HTTP method
 * @param url URL for POST request
 * @param headers HTTP headers
 * @param follow_location 1 - redirect on Location header
 * @param request POST request body
 * @param buffer request result
 * @return 0 on success
 */
int http_request(http_client_t *hc,
                 enum http_method method,
                 char *url,
                 struct curl_slist *headers,
                 int follow_location,
                 char *request,
                 buffer_t *buffer);

/**
 * Free HTTP client object
 * @brief http_free
 * @param hc HTTP client handler
 */
void http_free(http_client_t *hc);
