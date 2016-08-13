#include "cmrapi.h"
#include "list.h"
#include "filelist_cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <jansson.h>

#define FUSE_USE_VERSION 30
#include <fuse.h>
#include <libgen.h>
#include <errno.h>

struct cmr_t cmr;

static int cmr_getattr(const char *path, struct stat *stbuf) {
  if (strcmp(path, "/") == 0) {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
    return 0;
  }

  char *dir = dirname(strdup(path));

  if (strcmp(dir, cmr.filelist_cache.basedir) != 0) {
    struct list_t *lst;
    cmr_list_dir(&cmr, dir, &lst); // TODO: Ignore list in cmr_list_dir() on lst=NULL
  }

  memset(stbuf, 0, sizeof(struct stat));
  filelist_cache_data_t *data, query;

  strncpy(query.filename, path, sizeof(query.filename));
  data = (filelist_cache_data_t*)htable_find(&cmr.filelist_cache.files, &query);

  if (data == NULL)
    return -ENOENT;

  if (data->kind == FK_FOLDER) {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
    stbuf->st_size = data->filesize;
  }
  else
  {
    stbuf->st_mode = S_IFREG | 0644;
    stbuf->st_nlink = 1;
    stbuf->st_size = data->filesize;
    struct timespec ts = {.tv_sec=data->mtime, .tv_nsec=0};
    stbuf->st_mtimespec = ts;
    stbuf->st_ctimespec = ts;
    stbuf->st_atimespec = ts;
    stbuf->st_birthtimespec = ts;
  } 

  return 0;
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
  
  list_free(&lst);
  
  return 0;
}

int cmr_read(const char *filename, char *buffer, size_t size, off_t offset, struct fuse_file_info *file_info) {
  (void) file_info;

  return cmr_get_file(&cmr, filename, size, offset, buffer);
}

static struct fuse_operations cmr_ops = {
  .readdir= cmr_readdir,
  .getattr= cmr_getattr,
  .read= cmr_read,
};

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
  //

  int ret = fuse_main(argc, argv, &cmr_ops, NULL);

  cmr_finalize(&cmr);
  return ret;
}
