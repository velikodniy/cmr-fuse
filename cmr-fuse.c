#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <jansson.h>

#define BUFSIZE (10*1024)

struct buffer_t {
  char* data; 
  size_t length;
};

void buffer_init(struct buffer_t *buffer) {
  buffer->data = malloc(BUFSIZE);
  buffer->length = 0;
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
  memcpy(buffer->data + buffer->length, curl_buffer, real_size);
  buffer->length += real_size;
  buffer->data[buffer->length] = 0;
  return real_size;
}

int main(int argc, char **argv)
{
  if (argc < 2)
    exit(1);
  
  CURL *curl;
  CURLcode res;

  struct buffer_t buffer;
  buffer_init(&buffer);

  json_t *root;
  json_error_t error;

  root = json_load_file(argv[1], 0, &error);
    
  const char *username = json_string_value(json_object_get(root, "user"));
  const char *domain = json_string_value(json_object_get(root, "domain"));
  const char *password = json_string_value(json_object_get(root, "password"));

  char request[1024];
  snprintf(request, 1024, "Login=%s&Domain=%s&Password=%s", username, domain, password);

  curl_global_init(CURL_GLOBAL_DEFAULT);
  
  curl = curl_easy_init();
  if(!curl) {
    fprintf(stderr, "CURL: init error\n");
    exit(1);
  }

  //  curl_easy_setopt(curl, CURLOPT_USERAGENT, agent);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_null);
  curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "");
  
  curl_easy_setopt(curl, CURLOPT_POST, 1);
  curl_easy_setopt(curl, CURLOPT_URL, "https://auth.mail.ru/cgi-bin/auth");
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request);
  res = curl_easy_perform(curl);
  if(res != CURLE_OK)
    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
  
  curl_easy_setopt(curl, CURLOPT_POST, 0);
  curl_easy_setopt(curl, CURLOPT_URL, "https://auth.mail.ru/sdc?from=https://cloud.mail.ru/home");
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
  res = curl_easy_perform(curl);
  if(res != CURLE_OK)
    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
  
  struct curl_slist *chunk = NULL;
  chunk = curl_slist_append(chunk, "Accept: application/json");
  res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_buffer);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
  curl_easy_setopt(curl, CURLOPT_URL, "https://cloud.mail.ru/api/v2/tokens/csrf");
  res = curl_easy_perform(curl);
  if(res != CURLE_OK)
    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

  
  json_t *status, *body, *token;
  root = json_loads(buffer.data, 0, &error);
  
  char tok[128];
  
  status = json_object_get(root, "status");
  if (json_integer_value(status) == 200) {
    body = json_object_get(root, "body");
    token = json_object_get(body, "token");
    printf("token = %s\n", json_string_value(token));
    strncpy(tok, json_string_value(token), 128);
  }
  
  json_decref(root); 
  buffer_reset(&buffer);

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_buffer);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
  snprintf(request, 1024, "https://cloud.mail.ru/api/v2/folder?token=%s&home=/", tok);
  curl_easy_setopt(curl, CURLOPT_URL, request);
  res = curl_easy_perform(curl);
  if(res != CURLE_OK)
    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

  printf("%s\n", buffer.data);

  buffer_reset(&buffer);
  
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_buffer);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
  snprintf(request, 1024, "https://cloud.mail.ru/api/v2/dispatcher?token=%s", tok);
  curl_easy_setopt(curl, CURLOPT_URL, request);
  res = curl_easy_perform(curl);
  if(res != CURLE_OK)
    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

  printf("%s\n", buffer.data);

  buffer_reset(&buffer);

  root = json_loads(buffer.data, 0, &error);
  
  char shard_url[128];
  
  status = json_object_get(root, "status");
  if (json_integer_value(status) == 200) {
    json_t *body, *get, *first, *url;

    body = json_object_get(root, "body");
    get = json_object_get(body, "get");
    first = json_array_get(get, 0);
    url = json_object_get(first, "url");
    printf("URL = %s\n", json_string_value(url));
    strncpy(shard_url, json_string_value(url), 128);
  }
  
  json_decref(root); 

  strcat(shard_url, "test.txt");

  chunk = curl_slist_append(chunk, "Range: bytes=0-3");
 
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_buffer);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
  curl_easy_setopt(curl, CURLOPT_URL, shard_url);
  res = curl_easy_perform(curl);
  if(res != CURLE_OK)
    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

  printf("%s\n", buffer.data);

  curl_slist_free_all(chunk);
  curl_easy_cleanup(curl);
  
  buffer_free(&buffer);

  curl_global_cleanup();
  return 0;
}
