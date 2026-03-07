#ifndef ERI_SDK_H
#define ERI_SDK_H

#include "eri.h"

typedef void (*EriInit)(void);
typedef void (*EriUpdate)(void);

typedef struct EriSdkWidget {
    EriWidgetType type;
    u32 background_color;
    u32 num_children;
    struct EriSdkWidget **children;
    char *text;
    char *name;
} EriSdkWidget;

typedef struct EriSdkAlloc {
    u32 dummy;
} EriSdkAlloc;


u32 erisdk_parse_tlv(u8 *buf, EriTlv *tlv);
EriSdkWidget *erisdk_parse_tree(EriSdkAlloc *a, u8 *buf, usize size);

#endif /* ERI_SDK_H */
