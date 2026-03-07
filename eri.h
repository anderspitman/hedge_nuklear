#include <stdarg.h>

#ifndef NULL
#define NULL 0
#endif

typedef unsigned char u8;
typedef char u8_must_be_1_byte[sizeof(u8) == 1 ? 1 : -1];
typedef int i32;
typedef char i32_must_be_4_bytes[sizeof(i32) == 4 ? 1 : -1];
typedef unsigned int u32;
typedef char u32_must_be_4_bytes[sizeof(u32) == 4 ? 1 : -1];
/*typedef unsigned long u64;
typedef char u64_must_be_8_bytes[sizeof(u64) == 8 ? 1 : -1];
*/
typedef float f32;
typedef char f32_must_be_4_bytes[sizeof(f32) == 4 ? 1 : -1];
typedef double f64;
typedef char f64_must_be_8_bytes[sizeof(f64) == 8 ? 1 : -1];
typedef unsigned long usize;
typedef char usize_must_hold_pointer[sizeof(usize) >= sizeof(void*) ? 1 : -1];

typedef union {
    long l;
    double d;
    void *p;
} MaxAlign;

#define ARENA_ALIGNMENT sizeof(MaxAlign)

typedef struct Arena {
    usize offset;
    u8 *buf;
    usize size;
} Arena;


typedef struct Attr {
    u32 type;
    u32 num_values;
    void **values;
} Attr;


typedef struct EriTlv {
    u32 typ;
    u32 len;
    u8 *val;
} EriTlv;

typedef u32(*TlvCallback)(u8 *buf, usize off, void *user_data);

/* used to fix size for serialization */
typedef u32 EriSizedMsgType;
typedef u32 EriSizedWidgetType;
typedef u32 EriSizedTag;
typedef u32 EriSizedType;

typedef enum EriMessageAttr {
    ERI_MSG_ATTR_NAME = 0x50,
    ERI_MSG_ATTR_PATH,
    ERI_MSG_ATTR_TEXT
} EriMessageAttr;

typedef enum EriInMsgType {
    ERI_IN_MSG_WIDGET_PRESSED = 0x40,
    ERI_IN_MSG_TEXT_CHANGED
} EriInMsgType;

typedef enum EriOutMsgType {
    ERI_OUT_MSG_SET_TREE = 0x10
} EriOutMsgType;

typedef enum EriWidgetType {
    ERI_WIDGET_CONTAINER = 0x20,
    ERI_WIDGET_BUTTON,
    ERI_WIDGET_ROW,
    ERI_WIDGET_COLUMN,
    ERI_WIDGET_TEXTBOX
} EriWidgetType;

typedef enum EriWidgetAttr {
    ERI_WIDGET_ATTR_TEXT = 0x30,
    ERI_WIDGET_ATTR_NAME,
    ERI_WIDGET_ATTR_BG_COLOR,
    ERI_WIDGET_ATTR_CHILDREN
} EriWidgetAttr;

typedef struct EriWidget {
    EriWidgetType type;
    u32 background_color;
    u32 num_children;
    struct EriWidget **children;
    char *text;
    char *name;
} EriWidget;

typedef struct EriMsgWidgetPressed {
    char *path;
    char *name;
} EriMsgWidgetPressed;

typedef struct EriMsgTextChanged {
    char *path;
    char *text;
} EriMsgTextChanged;

typedef void (*MessageHandlerCallback)(void *state, u32 type, void *msg);


i32 eri_open(const char *path);
u32 eri_write(i32 handle, const u8 *bytes, u32 size);
void eri_widget_init(EriWidget *widget, EriWidgetType type);
void eri_set_tree(EriWidget *tree, const char *path);
void eri_register_message_handler(MessageHandlerCallback callback, void *state);
void eri_init(void);
u8 *eri_get_in_msg_buf(void);
u8 *eri_get_out_msg_buf(void);
u32 encode_tlv(u8 *buf, usize off, EriSizedType type, TlvCallback cb, void *user_data);
u32 eri_parse_tlv(u8 *buf, EriTlv *tlv);


void arena_init(Arena *arena, u8 *buf, usize size);
i32 arena_reset(Arena *arena, usize offset);

EriWidget *button(Arena *a, const char *txt);
EriWidget *edit_text(Arena *a, const char *txt);
EriWidget *row(Arena *a, u32 count, ...);
EriWidget *column(Arena *a, u32 count, ...);
