#pragma once

#include "cmrapi.h"

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
                 struct buffer_t *buffer);