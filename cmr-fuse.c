#include "cmrapi.h"
#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <jansson.h>

struct cmr_t cmr;

/*#define FUSE_USE_VERSION 30
#include <fuse.h>

 
static int cmr_getattr(const char *path, struct stat *stbuf)
{
  int res = 0;

  memset(stbuf, 0, sizeof(struct stat));
  if (strcmp(path, "/") == 0) {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
  } else {
    stbuf->st_mode = S_IFREG | 0444;
    stbuf->st_nlink = 1;
    stbuf->st_size = 0;
  } 

  return res;
}

static int cmr_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
  (void) offset;
  (void) fi;

  struct list_t *lst, *current;
  cmr_list_dir(&cmr, path, &lst);

  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);
  
  current = lst;
  while(current != NULL) {
    filler(buf, (char*)(current->data), NULL, 0);
    current = current->next;
  }
  
  //  list_free(&lst);
  
  return 0;
}

static struct fuse_operations cmr_ops = {
  .readdir= cmr_readdir,
  .getattr= cmr_getattr
  //  .read= hello_read,
}; */

int main(int argc, char **argv)
{
  /*  if (argc < 2) {
    printf("Usage: %s login.json", argv[0]);
    exit(1);
    }*/

  json_t *root;
  json_error_t error;
  root = json_load_file("login.json", 0, &error);
  const char *user = json_string_value(json_object_get(root, "user"));
  const char *domain = json_string_value(json_object_get(root, "domain"));
  const char *password = json_string_value(json_object_get(root, "password"));


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

  struct list_t *lst, *current;
  cmr_list_dir(&cmr, "/", &lst);

  current = lst;
  while(current != NULL) {
    printf("%s\n", (char*)(current->data));
    current = current->next;
  }
  
  list_free(&lst);
  cmr_finalize(&cmr);

  //  return fuse_main(argc, argv, &cmr_ops, NULL);
  return 0;
}
