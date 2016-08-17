#pragma once

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
 * @return
 */
CURL* curl_init(int verbose);

/**
 * Make HTTP POST request
 * @param curl CURL object
 * @param method HTTP method
 * @param url URL for POST request
 * @param headers HTTP headers
 * @param follow_location 1 - redirect on Location header
 * @param request POST request body
 * @param buffer request result
 * @return
 */
int curl_request(CURL *curl,
                 enum http_method method,
                 char *url,
                 struct curl_slist *headers,
                 int follow_location,
                 char *request,
                 buffer_t *buffer);
