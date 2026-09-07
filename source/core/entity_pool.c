#include "entity_pool.h"
#include <string.h>
#include <stddef.h>

static Entity s_pool[ENTITY_POOL_CAPACITY];
static bool   s_used[ENTITY_POOL_CAPACITY];
static int    s_count;

void entity_pool_reset(void) {
    memset(s_pool, 0, sizeof(s_pool));
    memset(s_used, 0, sizeof(s_used));
    s_count = 0;
}

Entity *entity_pool_alloc(EntityType type) {
    for (int i = 0; i < ENTITY_POOL_CAPACITY; i++) {
        if (!s_used[i]) {
            s_used[i] = true;
            s_count++;
            Entity *e = &s_pool[i];
            memset(e, 0, sizeof(Entity));
            e->type = type;
            e->alive = true;
            e->face = 1;
            return e;
        }
    }
    return NULL;
}

void entity_pool_free(Entity *e) {
    if (!e) return;
    ptrdiff_t idx = e - s_pool;
    if (idx < 0 || idx >= ENTITY_POOL_CAPACITY) return;
    if (s_used[idx]) {
        s_used[idx] = false;
        e->alive = false;
        s_count--;
    }
}

int entity_pool_count(void) { return s_count; }

Entity *entity_pool_at(int index) { return &s_pool[index]; }

void entity_pool_for_each(EntityVisitor visit, void *ctx) {
    for (int i = 0; i < ENTITY_POOL_CAPACITY; i++) {
        if (s_used[i]) {
            if (!visit(&s_pool[i], ctx)) return;
        }
    }
}
