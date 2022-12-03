#ifndef __ROUTER_H__
#define __ROUTER_H__

#include <string.h>
#include <stdbool.h>

struct node;

typedef struct router {
    struct node *root;
} router_t;

typedef struct router_match_ {
    void *value;
    char** slugs;
    size_t captures, max_captures;
    size_t *start, *end;
} router_match_t;

void router_init(router_t *r);

void router_free(router_t *r);

void router_add(router_t *r, const char *path, void *value);

router_match_t *router_match_new(size_t max_captures);

void router_match_free(router_match_t *m);

bool router_match(router_t *r, const char *path, router_match_t *out_match);

#endif
