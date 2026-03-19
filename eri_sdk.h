#ifndef ERI_SDK_H
#define ERI_SDK_H

#include <stdarg.h>

#include "eri.h"

typedef void (*EriInit)(void);
typedef void (*EriUpdate)(void);
typedef void (*MessageHandlerCallback)(void *state, u32 type, void *msg);

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

typedef struct EriSdkStr {
    char *ptr;
    u32 len;
} EriSdkStr;

typedef struct EriMsgWidgetPressed {
    char *path;
    char *name;
} EriMsgWidgetPressed;

typedef struct EriMsgTextChanged {
    char *path;
    char *text;
} EriMsgTextChanged;

typedef struct EriSdkWidget {
    EriWidgetType type;
    u32 background_color;
    u32 num_children;
    struct EriSdkWidget **children;
    EriSdkStr text;
    EriSdkStr name;
} EriSdkWidget;


void erisdk_arena_init(EriSdkArena *arena, u8 *buf, usize size);
void *erisdk_arena_alloc(EriSdkArena *arena, usize size);
i32 erisdk_arena_reset(EriSdkArena *arena, usize offset);
void mcpy(const void *src, void *dst, usize size);
u8 str_eq(const char *a, const char *b);
u32 str_len(const char *str);
void print(const char *msg);
char *fmts(char *str, const char *fmt, ...);
void print_fmt(const char *fmt, ...);

/*EriSdkStr erisdk_str_create(char *ptr, usize len);*/
char *erisdk_str_c(EriSdkStr *str);
usize erisdk_str_len(EriSdkStr *str);

typedef u32(*EriSdkTlvCallback)(u8 *buf, usize off, void *user_data);
u32 erisdk_encode_tlv(u8 *buf, usize off, EriType type, EriSdkTlvCallback cb, void *user_data);
u32 erisdk_encode_str(u8 *buf, usize off, void *user_data);
u32 erisdk_parse_tlv(u8 *buf, EriTlv *tlv);
EriSdkWidget *erisdk_parse_tree(EriSdkArena *a, u8 *buf, usize size);

/* App SDK */


EriSdkWidget *erisdk_button(EriSdkArena *a, const char *txt);
EriSdkWidget *erisdk_edit_text(EriSdkArena *a, const char *txt);
EriSdkWidget *erisdk_row(EriSdkArena *a, u32 count, ...);
EriSdkWidget *erisdk_column(EriSdkArena *a, u32 count, ...);
void eri_set_tree(EriSdkWidget *tree, const char *path);
void eri_register_message_handler(MessageHandlerCallback callback, void *state);
void eri_update(void);
u8 *eri_get_in_msg_buf(void);
u8 *eri_get_out_msg_buf(void);
i32 eri_open(const char *path);
u32 eri_write(i32 handle, const u8 *bytes, u32 size);

#endif /* ERI_SDK_H */
