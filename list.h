#pragma once

#include <stdlib.h>

struct list_t {
  void *data;
  struct list_t *next;
};

void list_init(struct list_t **list);
void list_append(struct list_t **list, void *data);
void list_free(struct list_t **list);
