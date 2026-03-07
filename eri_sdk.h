#ifndef ERI_SDK_H
#define ERI_SDK_H

#include "eri.h"

typedef void (*EriInit)(void);
typedef void (*EriUpdate)(void);

typedef union {
    long l;
    double d;
    void *p;
} MaxAlign;

#define ARENA_ALIGNMENT sizeof(MaxAlign)

typedef struct EriSdkArena {
    usize offset;
    u8 *buf;
    usize size;
} EriSdkArena;


typedef struct EriSdkWidget {
    EriWidgetType type;
    u32 background_color;
    u32 num_children;
    struct EriSdkWidget **children;
    char *text;
    char *name;
} EriSdkWidget;


void erisdk_arena_init(EriSdkArena *arena, u8 *buf, usize size);
void *erisdk_arena_alloc(EriSdkArena *arena, usize size);
i32 erisdk_arena_reset(EriSdkArena *arena, usize offset);

u32 erisdk_parse_tlv(u8 *buf, EriTlv *tlv);
EriSdkWidget *erisdk_parse_tree(EriSdkArena *a, u8 *buf, usize size);

#endif /* ERI_SDK_H */
