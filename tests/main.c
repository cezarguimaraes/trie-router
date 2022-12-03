#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "../src/router.c"

int main(int argc, char *argv[]) {
    router_t r;
    router_init(&r);

    node_t *index = _router_add(&r, "/");
    node_t *page1 = _router_add(&r, "/page1");
    node_t *page2 = _router_add(&r, "/page/page2");
    node_t *catchAll = _router_add(&r, "/catch-all/*");
    node_t *innerCatch = _router_add(&r, "/*/inner-catch");
    node_t *multipleCatch = _router_add(&r, "/multiple/*/*");
    node_t *oneSlug = _router_add_slugs(&r, "/:product/p");
    node_t *oneConflictingSlug = _router_add_slugs(&r, "/:blog-post/b");
    /* node_t *twoSlugs =*/ _router_add_slugs(&r, "/:month/:day/updates");
    node_t *priorityOne = _router_add(&r, "/foo/*");
    /*node_t *priorityTwo =*/ _router_add(&r, "/*/baz");

    /*node_t *conflictOne =*/ _router_add(&r, "/foo/*/a");
    node_t *conflictTwo = _router_add(&r, "/*/baz/b");


    const node_t *p = _router_exact_find(&r, "/");
    assert(p == index);
    assert(p->match);

    p = _router_exact_find(&r, "/pa");
    assert(p != NULL);
    assert(!p->match);

    p = _router_exact_find(&r, "/page1");
    assert(p == page1);
    assert(p->match);

    p = _router_exact_find(&r, "/page/page2");
    assert(p == page2);
    assert(p->match);

    p = _router_match(&r, "/catch-all/ab");
    assert(p == catchAll);
    assert(p->match);

    p = _router_match(&r, "/catch-all/ab/nope");
    assert(p == NULL);

    p = _router_match(&r, "/teste/inner-catch");
    assert(p == innerCatch);
    assert(p->match);

    p = _router_match(&r, "/teste/nope/inner-catch");
    assert(p == NULL);

    p = _router_match(&r, "/multiple/a/b");
    assert(p == multipleCatch);
    assert(p->match);

    p = _router_match(&r, "/multiple/d/x");
    assert(p == multipleCatch);
    assert(p->match);

    p = _router_match(&r, "/multiple/d/x/nope");
    assert(p == NULL);

    const size_t MAX_MATCHES = 10;
    size_t match_start[MAX_MATCHES], match_end[MAX_MATCHES];

    size_t matches;
    p = _router_match_with_captures(&r, "/my-product-slug/p", match_start, match_end, &matches, MAX_MATCHES);
    assert(p == oneSlug);
    assert(p->match);
    // matches my-product-slug
    assert(match_start[0] == 1);
    assert(match_end[0] == 16);
    assert(matches == 1); // only 1 capture
    assert(strcmp(p->slugs[0], "product") == 0);

    p = _router_match_with_captures(&r, "/my-blog-post-slug/b", match_start, match_end, &matches, MAX_MATCHES);
    assert(p == oneConflictingSlug);
    assert(p->match);
    // matches my-product-slug
    assert(match_start[0] == 1);
    assert(match_end[0] == 18);
    assert(matches == 1); // only 1 capture
    assert(strcmp(p->slugs[0], "blog-post") == 0);

    p = _router_match_with_captures(&r, "/foo/baz", match_start, match_end, &matches, MAX_MATCHES);
    assert(p == priorityOne);
    assert(p->match);
    assert(match_start[0] == 5);
    assert(match_end[0] == 8);
    assert(matches == 1); // only 1 capture
    assert(p->slugs == NULL);

    p = _router_match_with_captures(&r, "/foo/baz/b", match_start, match_end, &matches, MAX_MATCHES);
    assert(p == conflictTwo);
    assert(p->match);
    // matches my-product-slug
    assert(match_start[0] == 1);
    assert(match_end[0] == 4);
    assert(matches == 1); // only 1 capture
    assert(p->slugs == NULL);

    router_match_t *match = router_match_new(MAX_MATCHES);
    assert(router_match(&r, "/january/5/updates", match));
    assert(match->captures == 2);
    assert(match->start[0] == 1 && match->end[0] == 8); // matched january
    assert(match->start[1] == 9 && match->end[1] == 10); // matched 5
    assert(strcmp(match->slugs[0], "month") == 0);
    assert(strcmp(match->slugs[1], "day") == 0);

    router_match_free(match);
    router_free(&r);
    return 0;
}