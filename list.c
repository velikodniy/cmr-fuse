#include "list.h"

void list_init(struct list_t **list) {
  *list = NULL;
}

void list_append(struct list_t **list, void *data) {
  struct list_t *head = *list;
  *list = malloc(sizeof(struct list_t));
  (*list)->next = head;
  (*list)->data = data;
}

void list_free(struct list_t **list) {
  struct list_t *next;
  while(*list != NULL) {
    next = (*list)->next;
    free((*list)->data);
    free(*list);
    *list = next;
  }
}
