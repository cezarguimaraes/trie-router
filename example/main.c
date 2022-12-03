#include <stdio.h>

#include "../src/router.h"

int main(int argc, char *argv[]) {
    router_t r;
    router_init(&r);

    router_add(&r, "/", "index page");
    router_add(&r, "/:product/p", "product page");
    router_add(&r, "/:post/b", "blog post page");
    router_add(&r, "/:month/:day/b", "day blog page");
    router_add(&r, "/*/s", "search page");

    const char *urls[] = {
        "/",
        "/red-blouse/p",
        "/blue-blouse/p",
        "/making-a-trie-router/b",
        "/january/1/b",
        "/blouses/s",
        "/foo/s",
        "/foo/bar/s"
    };

    router_match_t *m = router_match_new(20);

    const size_t URL_COUNT = sizeof(urls) / sizeof(*urls);
    for (size_t i = 0; i < URL_COUNT; i++) {
        bool found = router_match(&r, urls[i], m);
        printf("match for URL %s: %s\n", urls[i], found ? (char*)m->value : "NOT FOUND");
        if (found) {
            for (size_t j = 0; j < m->captures; j++) {
                printf("\tcapture %lu (%s): %.*s\n", j, m->slugs ? m->slugs[j] : "no slug", (int)(m->end[j] - m->start[j]), urls[i] + m->start[j]);
            }
        }
    }

    router_match_free(m);
    router_free(&r);
}
