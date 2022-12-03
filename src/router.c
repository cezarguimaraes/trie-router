#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "router.h"

#define WILDCARD_CHAR '*'
#define SLUG_BEGIN_CHAR ':'

typedef struct node {
    // whether this is a final node
    bool match;
    // names of slugs up to this node (node is always final when slugs are present)
    // NULL terminated
    char **slugs;
    // could be a linked list to reduce memory usage
    // or ideally a hashmap
    struct node *next[256]; 

    void *value; 
} node_t;

void router_init(router_t *r) {
    r->root = calloc(1, sizeof(*r->root));
}

node_t *_router_traverse(node_t *current, const char *path, size_t at, size_t until, bool addMissingNodes) {
    if (at == until) {
        return current;
    }

    node_t **next = &current->next[(size_t)path[at]];
    if (*next == NULL) {
        if (!addMissingNodes) {
            return NULL;
        }
        *next = calloc(1, sizeof(node_t)); 
    }

    return _router_traverse(*next, path, at+1, until, addMissingNodes);
}

node_t *_router_add(router_t *r, const char* path) {
    assert(path != NULL);
    node_t *finalNode = _router_traverse(r->root, path, 0, strlen(path), true);
    // if finalNode.match isn't false we are inserting a duplicate path 
    assert(finalNode->match == false);

    finalNode->match = true;
    return finalNode;
}

/* same as _router_add but converts slugs to wildcard segments and stores slug names on final node */
node_t *_router_add_slugs(router_t *r, const char* path) {
    size_t pathLen = strlen(path);
    char *convertedPath = calloc(1, pathLen+1);
    size_t at = 0;
    char **slugs = NULL;
    size_t slugCount = 0;
    for (size_t i = 0; i < pathLen; ) {
        if (path[i] == SLUG_BEGIN_CHAR) {
            // can be improved to reduce number of allocations
            slugs = realloc(slugs, sizeof(*slugs) * (slugCount + 2));
            slugs[slugCount + 1] = NULL;

            size_t continueAt = i;
            while (++continueAt < pathLen && path[continueAt] != '/');

            slugs[slugCount] = malloc(continueAt - i);
            memcpy(slugs[slugCount], path + i + 1, continueAt - i - 1);
            slugs[slugCount][continueAt - i - 1] = '\0';

            slugCount += 1;

            convertedPath[at++] = WILDCARD_CHAR;
            i = continueAt;
        } else {
            convertedPath[at++] = path[i];
            i += 1;
        }
    }

    node_t *finalNode = _router_add(r, convertedPath);
    finalNode->slugs = slugs;

    free(convertedPath);

    return finalNode;
}

node_t *_router_exact_find(router_t *r, const char* path) {
    return _router_traverse(r->root, path, 0, strlen(path), false);
}

void _router_node_free(node_t *current) {
    if (current == NULL) {
        return;
    }

    for (size_t i = 0; i < 256; i++) {
        _router_node_free(current->next[i]);
    }

    if (current->slugs != NULL) {
        for (char **slugPtr = current->slugs; *slugPtr != NULL; slugPtr++) {
            free(*slugPtr);
        }
        free(current->slugs);
    }

    free(current);
}

// does not free r itself
void router_free(router_t *r) {
    _router_node_free(r->root);
}

const node_t *_router_match_(
    const node_t *const current, 
    const char *path, 
    const size_t at, const size_t until,
    size_t *const out_match_start, size_t *const out_match_end, size_t *const out_matches,
    size_t const cur_match, size_t const max_matches
) {
    // params were made const to avoid mistakes if this function is ever converted to backtrack
    if (at == until) {
        if (out_matches != NULL) {
            *out_matches = cur_match;
        }
        return current;
    }

    // first attempt matching exactly
    node_t *next = current->next[(size_t)path[at]];
    if (next != NULL) {
        const node_t *exact = _router_match_(
            next,
            path,
            at+1, until,
            out_match_start, out_match_end, out_matches,
            cur_match, max_matches
        );
        if (exact != NULL && exact->match) {
            return exact;
        }
    }

    // match wildcards
    node_t *wildcardNode = current->next[(size_t)WILDCARD_CHAR];
    if (wildcardNode != NULL && at > 0 && path[at-1] == '/') {
        size_t continueAt = at;
        while (++continueAt < until && path[continueAt] != '/');
        if (cur_match < max_matches) {
            *out_match_start = at;
            *out_match_end = continueAt;
            return _router_match_(
                wildcardNode,
                path,
                continueAt, until,
                out_match_start+1, out_match_end+1, out_matches,
                cur_match+1, max_matches
            );
        } else {
            return _router_match_(
                wildcardNode,
                path,
                continueAt, until,
                out_match_start, out_match_end, out_matches,
                cur_match, max_matches
            );
        }
    }

    return NULL;
}

const node_t *_router_match(router_t *r, const char *path) {
    return _router_match_(r->root, path, 0, strlen(path), NULL, NULL, NULL, 0, 0);
}

const node_t *_router_match_with_captures(router_t *r, const char *path, size_t *out_match_start, size_t *out_match_end, size_t *out_matches, size_t max_matches) {
    return _router_match_(r->root, path, 0, strlen(path), out_match_start, out_match_end, out_matches, 0, max_matches);
}

router_match_t *router_match_new(size_t max_captures) {
    router_match_t *m = malloc(sizeof(*m));
    m->start = malloc(sizeof(*m->start) * max_captures);
    m->end = malloc(sizeof(*m->end) * max_captures);
    m->value = NULL;
    m->captures = 0;
    m->max_captures = max_captures;
    m->slugs = NULL;
    return m;
}

void router_match_free(router_match_t *m) {
    free(m->start);
    free(m->end);
    free(m);
}

bool router_match(router_t *r, const char *path, router_match_t *out_match) {
    const node_t *found = _router_match_with_captures(r, path, out_match->start, out_match->end, &out_match->captures, out_match->max_captures);
    if (found == NULL || !found->match) {
        return false;
    }
    out_match->value = found->value;
    out_match->slugs = found->slugs;
    return true;
}

void router_add(router_t *r, const char *path, void *value) {
    node_t *node = _router_add_slugs(r, path);
    node->value = value;
}
