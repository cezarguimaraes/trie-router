# trie-router
Simple C router with wildcard and slug capture support.

## Usage

- Initialize a `router`
```c
router_t r;
router_init(&r);
```

- Add routes
```c
// The third parameter of router_add is a void pointer.
// The pointer is returned whenever the specific route matches, so pass in whatever you need
router_add(&r, "/", "index page");
router_add(&r, "/:product/p", "product page");
router_add(&r, "/:post/b", "blog post page");
router_add(&r, "/:month/:day/b", "day blog page");
router_add(&r, "/*/s", "search page");
```

- Initialize a `router_match` structure
```c
router_match_t *m = router_match_new(/* MAX MATCHES: */ 20);
```

- Match URLs
```c
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

const size_t URL_COUNT = sizeof(urls) / sizeof(*urls);
for (size_t i = 0; i < URL_COUNT; i++) {
    bool found = router_match(&r, urls[i], m);
    printf("match for URL %s: %s\n", urls[i], found ? (char*)m->value : "NOT FOUND");
    if (found) {
        for (size_t j = 0; j < m->captures; j++) {
            printf("\tcapture %lu (%s): %.*s\n", j, 
              m->slugs ? m->slugs[j] : "no slug", 
              (int)(m->end[j] - m->start[j]), 
              urls[i] + m->start[j]
            );
        }
    }
}
```

  Output for this example:
  ```
  match for URL /: index page
  match for URL /red-blouse/p: product page
          capture 0 (product): red-blouse
  match for URL /blue-blouse/p: product page
          capture 0 (product): blue-blouse
  match for URL /making-a-trie-router/b: blog post page
          capture 0 (post): making-a-trie-router
  match for URL /january/1/b: day blog page
          capture 0 (month): january
          capture 1 (day): 1
  match for URL /blouses/s: search page
          capture 0 (no slug): blouses
  match for URL /foo/s: search page
          capture 0 (no slug): foo
  match for URL /foo/bar/s: NOT FOUND
  ```

- Free up memory
```c
router_match_free(m);
router_free(&r);
```

Check out the [full example](./example/main.c)

## Details

Time complexity is, for the most part, linear on the size of the input string of `route_match`. This will hold true as long as your wildcards and slugs are exclusively on the last segment of your routes.

The complexity breaks apart when you have conflicting routes that might "unmatch" at the end, such as:
- `/foo/*/one`
- `/*/bar/two`

and try to match `/foo/bar/two`. When trying to match this string, priority is given for exact matches, 
so the Trie traversal algorithm will first try to match it with `/foo/*/one`. 
This won't work and the algorithm needs to backtrack to try matching with the inner wildcard on `/*/bar/two`.

This is essentially the same problem that arises when using the `Kleene Star` in regular expressions such as:
```js
let r1 = new RegExp(/^a\s* \s*b$/)
let r2 = new RegExp(/^a\s+b$/)

let spaces = ' '.repeat(10000)

let S = 'a' + spaces + 'c'

r1.exec(S) // O(|S|**2)
r2.exec(S) // O(|S|)
``` 

Try it on https://jsbench.me/ and one can see `r1` is around 5000 times slower.

That said, the worst case complexity of the Trie traversal implemented here is still linear on the **Trie** size, that is to say, linear on the sum of the lengths of your declared routes. So our match, on the absolute worst case, is not worse than executing a regular expression over all of your routes.

---

Inspired by `Imran Rahman` [video](https://www.youtube.com/watch?v=cEH_ipqHbUw) on implementing a web server in C. 
