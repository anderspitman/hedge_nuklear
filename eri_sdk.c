#include <stdio.h>

#include "eri_sdk.h"

void erisdk_arena_init(EriSdkArena *arena, u8 *buf, usize size) {
    arena->offset = 0;
    arena->buf = buf;
    arena->size = size;
}

void *erisdk_arena_alloc(EriSdkArena *arena, usize size) {
    usize padding = 0;
    usize p = (usize)&arena->buf[arena->offset];
    usize aligned = (p + (ARENA_ALIGNMENT - 1)) & ~(ARENA_ALIGNMENT - 1);

    padding = aligned - p;

    if (arena->offset + size + padding > arena->size) {
        return 0;
    }

    arena->offset += size + padding;

    return (void *)aligned;
}

i32 erisdk_arena_reset(EriSdkArena *arena, usize offset) {
    if (offset >= arena->size) {
        return -1;
    }

    arena->offset = offset;

    return 0;
}

static void mcpy(const void *src, void *dst, usize size) {
    usize i = 0;
    const u8 *s = (const u8 *)src;
    u8 *d = (u8 *)dst;
    for (i = 0; i < size; i++) {
        d[i] = s[i]; 
    }
}

u32 erisdk_parse_tlv(u8 *buf, EriTlv *tlv) {
    u32 off = 0;

    mcpy(&buf[off], &tlv->typ, 4);
    off += 4;
    mcpy(&buf[off], &tlv->len, 4);
    off += 4;
    tlv->val = &buf[off];
    off += tlv->len;

    return off;
}

u32 erisdk_parse_widget(EriSdkArena *a, u8 *buf, EriSdkWidget **wid, usize depth) {
    EriTlv tlv = {0};
    usize off = 0;
    usize attr_off = 0;
    usize i = 0;

    *wid = erisdk_arena_alloc(a, sizeof(EriSdkWidget));

    off += erisdk_parse_tlv(buf, &tlv);

    (*wid)->type = tlv.typ;

    for (i = 0; i < (depth * 2); i++) {
        printf(" ");
    }
    printf("0x%x\n", tlv.typ);

    while (attr_off < tlv.len) {
        EriTlv attr_tlv = {0};

        attr_off += erisdk_parse_tlv(&tlv.val[attr_off], &attr_tlv);

        switch (attr_tlv.typ) {
            case ERI_WIDGET_ATTR_NAME: {
                (*wid)->name = attr_tlv.val;
                break;
            }
            case ERI_WIDGET_ATTR_TEXT: {
                (*wid)->text = attr_tlv.val;
                break;
            }
            case ERI_WIDGET_ATTR_CHILDREN: {
                usize child_off = 0;
                EriSdkWidget **children = 0;
                u32 num_children = 0;
                usize cidx = 0;

                while (child_off < attr_tlv.len) {
                    EriTlv dummy_tlv;
                    child_off += erisdk_parse_tlv(&attr_tlv.val[child_off], &dummy_tlv);
                    num_children += 1;
                }

                children = erisdk_arena_alloc(a, num_children * sizeof(EriSdkWidget));

                child_off = 0;

                for (cidx = 0; cidx < num_children && child_off < attr_tlv.len; cidx += 1) {
                    child_off += erisdk_parse_widget(a, &attr_tlv.val[child_off], &children[cidx], depth + 1);
                }

                (*wid)->children = children;
                (*wid)->num_children = num_children;

                break;
            }
            default: {
                break;
            }
        }
    }
    
    return off;
}


EriSdkWidget *erisdk_parse_tree(EriSdkArena *a, u8 *buf, usize size) {
    usize off = 0;
    EriSdkWidget *wid = 0;

    (void)a;

    while (off < size) {
        off += erisdk_parse_widget(a, buf, &wid, 0);
    }

    return wid;
}
