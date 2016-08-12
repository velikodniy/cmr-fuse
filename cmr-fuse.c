#include "cmrapi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <jansson.h>


int main(int argc, char **argv)
{
  if (argc < 2) {
    printf("Usage: %s login.json", argv[0]);
    exit(1);
  }

  json_t *root;
  json_error_t error;
  root = json_load_file(argv[1], 0, &error);
  const char *user = json_string_value(json_object_get(root, "user"));
  const char *domain = json_string_value(json_object_get(root, "domain"));
  const char *password = json_string_value(json_object_get(root, "password"));

  struct cmr_t cmr;

  cmr_init(&cmr, user, domain, password);
  cmr_login(&cmr);
  cmr_sdc_cookies(&cmr);

  cmr_get_token(&cmr);
  printf("Token=%s\n", cmr.token);

  cmr_get_shard_urls(&cmr);
  printf("DL=%s\n", cmr.download);
  printf("UL=%s\n", cmr.upload);


  char buf[5] = {0};
  cmr_get_file(&cmr, "/test.txt", 4, 0, buf);
  printf("%s\n", buf);
  
  cmr_finalize(&cmr);
  
  return 0;
}
