#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <jansson.h>
#include <libgen.h>
#define FUSE_USE_VERSION 30
#include <fuse.h>

#include "cmrapi.h"
#include "list.h"
#include "filelist_cache.h"

#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)
#ifdef PROJECT_VERSION
  #define VERSION STR(PROJECT_VERSION)
#else
  #define VERSION "???"
#endif

struct cmr_t cmr;

static int cmr_getattr(const char *path, struct stat *stbuf) {
  if (strcmp(path, "/") == 0) {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
    return 0;
  }

  char *dir = dirname(strdup(path));

  if (strcmp(dir, cmr.filelist_cache->basedir) != 0) {
    struct list_t *lst;
    cmr_list_dir(&cmr, dir, &lst); // TODO: Ignore list in cmr_list_dir() on lst=NULL
  }

  memset(stbuf, 0, sizeof(struct stat));

  filelist_cache_data_t *data;
  data = filelist_cache_find(cmr.filelist_cache, path);

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
#ifdef __APPLE__
    struct timespec ts = {.tv_sec=data->mtime, .tv_nsec=0};
    stbuf->st_mtimespec = ts;
    stbuf->st_ctimespec = ts;
    stbuf->st_atimespec = ts;
    stbuf->st_birthtimespec = ts;
#else
    time_t ts = data->mtime;
    stbuf->st_mtime = ts;
    stbuf->st_ctime = ts;
    stbuf->st_atime = ts;
#endif
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

struct app_config {
  char *config_file_name;
};

enum {
  KEY_HELP,
  KEY_VERSION,
};

#define APP_OPT(t, p, v) { t, offsetof(struct app_config, p), v }

static struct fuse_opt myfs_opts[] = {
    APP_OPT("config=%s",           config_file_name, 0),

    FUSE_OPT_KEY("-V",             KEY_VERSION),
    FUSE_OPT_KEY("--version",      KEY_VERSION),
    FUSE_OPT_KEY("-h",             KEY_HELP),
    FUSE_OPT_KEY("--help",         KEY_HELP),
    FUSE_OPT_END
};

static int app_opt_proc(void *data, const char *arg, int key, struct fuse_args *outargs)
{
  (void) data;
  (void) arg;
  switch (key) {
    case KEY_HELP:
      fprintf(stderr,
              "usage: %s mountpoint [options]\n"
                  "\n"
                  "general options:\n"
                  "    -o opt,[opt...]  mount options\n"
                  "    -h   --help      print help\n"
                  "    -V   --version   print version\n"
                  "\n"
                  "app options:\n"
                  "    -o config=STRING\n"
          , outargs->argv[0]);
      fuse_opt_add_arg(outargs, "-ho");
      fuse_main(outargs->argc, outargs->argv, &cmr_ops, NULL);
      exit(EXIT_SUCCESS);

    case KEY_VERSION:
      fprintf(stderr, "%s version %s\n", outargs->argv[0], VERSION);
      fuse_opt_add_arg(outargs, "--version");
      fuse_main(outargs->argc, outargs->argv, &cmr_ops, NULL);
      exit(EXIT_SUCCESS);
  }
  return 1;
}

void fail(const char* message) {
  fprintf(stderr, "ERROR: %s\n", message);
  exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
  struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
  struct app_config conf;

  conf.config_file_name = NULL;
  fuse_opt_parse(&args, &conf, myfs_opts, app_opt_proc);

  if (conf.config_file_name == NULL) fail("You must set a config file with \"-o config=FILE\" option");

  json_t *root;
  json_error_t error;
  root = json_load_file(conf.config_file_name, 0, &error);
  if (root == NULL) fail("Cannot open or parse config file");

  const char *user = json_string_value(json_object_get(root, "user"));
  const char *domain = json_string_value(json_object_get(root, "domain"));
  const char *password = json_string_value(json_object_get(root, "password"));

  if (user == NULL || domain == NULL || password == NULL)
    fail("There isn't user/domain/password setting in the config file");

  cmr_init(&cmr, user, domain, password);
  cmr_login(&cmr);
  cmr_sdc_cookies(&cmr);
  cmr_get_token(&cmr);
  cmr_get_shard_urls(&cmr);

  int ret = fuse_main(args.argc, args.argv, &cmr_ops, NULL);

  cmr_finalize(&cmr);
  return ret;
}
