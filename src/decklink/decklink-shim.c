#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#define DECKLINK_FRAMEWORK "/Library/Frameworks/DeckLinkAPI.framework/DeckLinkAPI"

typedef void* (*CreateIteratorFunc)(void);

static void* find_versioned_iterator(void) {
    void* handle = dlopen(DECKLINK_FRAMEWORK, RTLD_LAZY | RTLD_GLOBAL);
    if (!handle) return NULL;

    static const char* names[] = {
        "CreateDeckLinkIteratorInstance_0004",
        "CreateDeckLinkIteratorInstance_0003",
        "CreateDeckLinkIteratorInstance_0002",
        "CreateDeckLinkIterator",
        NULL
    };

    for (int i = 0; names[i]; i++) {
        CreateIteratorFunc fn = (CreateIteratorFunc)dlsym(handle, names[i]);
        if (fn) return (void*)fn;
    }
    return NULL;
}

void* CreateDeckLinkIteratorInstance(void) {
    static CreateIteratorFunc cached_fn = NULL;
    static int resolved = 0;

    if (!resolved) {
        cached_fn = (CreateIteratorFunc)find_versioned_iterator();
        resolved = 1;
        if (!cached_fn) {
            fprintf(stderr, "[DeckLink Shim] No compatible CreateDeckLinkIteratorInstance found in framework\n");
        }
    }

    return cached_fn ? cached_fn() : NULL;
}
