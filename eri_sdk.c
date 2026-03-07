#include <stdio.h>

#include "eri_sdk.h"

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

u32 erisdk_parse_widget(EriSdkAlloc *a, u8 *buf, EriSdkWidget *wid, usize depth) {
    EriTlv tlv = {0};
    usize off = 0;
    usize attr_off = 0;
    usize i = 0;

    (void)a;
    (void)wid;

    off += erisdk_parse_tlv(buf, &tlv);

    for (i = 0; i < (depth * 2); i++) {
        printf(" ");
    }
    printf("0x%x\n", tlv.typ);

    while (attr_off < tlv.len) {
        EriTlv attr_tlv = {0};

        attr_off += erisdk_parse_tlv(&tlv.val[attr_off], &attr_tlv);

        switch (attr_tlv.typ) {
            case ERI_WIDGET_ATTR_NAME: {
                break;
            }
            case ERI_WIDGET_ATTR_TEXT: {
                break;
            }
            case ERI_WIDGET_ATTR_CHILDREN: {
                usize child_off = 0;
                EriSdkWidget *child;

                while (child_off < attr_tlv.len) {
                    child_off += erisdk_parse_widget(a, &attr_tlv.val[child_off], child, depth + 1);
                }

                break;
            }
            default: {
                break;
            }
        }
    }
    
    return off;
}


EriSdkWidget *erisdk_parse_tree(EriSdkAlloc *a, u8 *buf, usize size) {
    usize off = 0;
    EriSdkWidget *wid = 0;

    (void)a;

    while (off < size) {
        off += erisdk_parse_widget(a, buf, wid, 0);
    }
    printf("off: %lu\n", off);

    return 0;
}
