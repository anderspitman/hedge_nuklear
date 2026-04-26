#include <stdarg.h>
#include "eri_sdk.h"

static u8 in_msg_buf[1*1024*1024];
static u8 out_msg_buf[1*1024*1024];
static usize out_offset = 0;
static i32 print_file = -1;

static MessageHandlerCallback message_handler_callback = 0;
static void *message_handler_callback_state = 0;
static char str_buf[1024];

u8 *erisdk_get_in_msg_buf(void) {
    return in_msg_buf;
}

u8 *erisdk_get_out_msg_buf(void) {
    return out_msg_buf;
}

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

void erisdk_mcpy(const void *src, void *dst, usize size) {
    usize i = 0;
    const u8 *s = (const u8 *)src;
    u8 *d = (u8 *)dst;
    for (i = 0; i < size; i++) {
        d[i] = s[i]; 
    }
}

static EriSdkStr erisdk_str_create(EriSdkArena *a, char *in_str, usize len) {
    EriSdkStr str = {0};
    str.ptr = erisdk_arena_alloc(a, len + 1);
    erisdk_mcpy(in_str, str.ptr, len);
    str.ptr[len] = '\0';
    str.len = len;
    return str;
}

usize erisdk_str_len(EriSdkStr *str) {
    return str->len;
}

char *erisdk_str_c(EriSdkStr *str) {
    return str->ptr;
}

u8 erisdk_str_eq(const char *a, const char *b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return (u8)*a == (u8)*b;
}

u32 erisdk_cstr_len(const char *str) {
    u32 len = 0;
    while (str[len] != '\0') {
        len += 1;
    }
    return len;
}

void erisdk_print(const char *msg) {
    u32 len = 0;
    if (-1 == print_file) {
        print_file = erisdk_open("/dev/stdout");
    }
    len = erisdk_cstr_len(msg);
    erisdk_write(print_file, (u8 *)msg, len);
}

static i32 itoa(i32 value, char *buf) {
    char *p = buf;
    char *start = buf;
    i32 v;
    i32 count;
    char c;

    if (value < 0) {
        v = -(value + 1);
        v = v + 1;
    } else {
        v = value;
    }

    do {
        *p++ = (char)('0' + (v % 10));
        v /= 10;
    } while (v != 0);

    if (value < 0) {
        *p++ = '-';
    }

    count = (i32)(p - buf);
    p--;
    while (start < p) {
        c = *start;
        *start = *p;
        *p = c;
        start++;
        p--;
    }

    return count;
}

static char *vfmts(char *str, const char *fmt, va_list args) {
    u32 i = 0;
    u32 str_idx = 0;
    i32 val = -1;

    for (i = 0; fmt[i] != 0; i++) {
        if (fmt[i] == '%') {
            val = va_arg(args, i32);
            str_idx += (u32)itoa(val, &str[str_idx]);
        } else {
            str[str_idx++] = fmt[i];
        }
    }

    str[str_idx] = 0;
    return str;
}

char *erisdk_fmts(char *str, const char *fmt, ...) {
    char *result;
    va_list args;

    va_start(args, fmt);
    result = vfmts(str, fmt, args);
    va_end(args);

    return result;
}

void erisdk_print_fmt(const char *fmt, ...) {
    char buf[256];
    va_list args;

    va_start(args, fmt);
    vfmts(buf, fmt, args);
    va_end(args);

    erisdk_print(buf);
}

static u32 encode_type(u8 *buf, u32 off, EriType type) {
    erisdk_mcpy(&type, &buf[off], sizeof(EriType));
    off += sizeof(EriType);
    return off;
}

u32 erisdk_encode_tlv(u8 *buf, usize off, EriType type, EriSdkTlvCallback cb, void *user_data) {

    usize len = 0;
    usize len_off = 0;

    off = encode_type(buf, off, type);

    /* save offset of length and reserve space for it */
    len_off = off;
    off += sizeof(u32);

    off = cb(buf, off, user_data);

    /* write length */
    len = off - len_off - sizeof(u32);
    erisdk_mcpy(&len, &buf[len_off], sizeof(u32));

    /* TODO: bad idea? */
    buf[off] = 0;

    return off;
}

u32 erisdk_encode_str(u8 *buf, usize off, void *user_data) {
    EriSdkStr *str = (EriSdkStr *)user_data;
    usize len = erisdk_str_len(str);
    erisdk_mcpy(str->ptr, &buf[off], len);
    off += len;
    return off;
}

u32 erisdk_parse_tlv(u8 *buf, EriTlv *tlv) {
    u32 off = 0;

    erisdk_mcpy(&buf[off], &tlv->typ, 4);
    off += 4;
    erisdk_mcpy(&buf[off], &tlv->len, 4);
    off += 4;
    tlv->val = &buf[off];
    off += tlv->len;

    return off;
}

static u32 erisdk_parse_widget(EriSdkArena *a, u8 *buf, EriSdkWidget **wid, usize depth) {
    EriTlv tlv = {0};
    usize off = 0;
    usize attr_off = 0;

    *wid = erisdk_arena_alloc(a, sizeof(EriSdkWidget));

    off += erisdk_parse_tlv(buf, &tlv);

    (*wid)->type = tlv.typ;
    (void)depth;

    while (attr_off < tlv.len) {
        EriTlv attr_tlv = {0};

        attr_off += erisdk_parse_tlv(&tlv.val[attr_off], &attr_tlv);

        switch (attr_tlv.typ) {
            case ERI_WIDGET_ATTR_NAME: {
                (*wid)->name = erisdk_str_create(a, (char *)attr_tlv.val, attr_tlv.len);
                break;
            }
            case ERI_WIDGET_ATTR_TEXT: {
                (*wid)->text = erisdk_str_create(a, (char *)attr_tlv.val, attr_tlv.len);
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

static void erisdk_widget_init(EriSdkWidget *widget, EriWidgetType type) {
    widget->type = type;
    widget->background_color = 0;
    widget->num_children = 0;
    widget->children = 0;
    widget->text.ptr = 0;
    widget->text.len = 0;
    widget->name.ptr = 0;
    widget->name.len = 0;
}

static EriSdkWidget *erisdk_widget(EriSdkArena *a, EriWidgetType type) {
    EriSdkWidget *widget = erisdk_arena_alloc(a, sizeof(EriSdkWidget));
    if (widget == 0) {
        return 0;
    }

    erisdk_widget_init(widget, type);
    return widget;
}

EriSdkWidget *erisdk_button(EriSdkArena *a, const char *txt) {
    EriSdkWidget *widget = erisdk_widget(a, ERI_WIDGET_BUTTON);
    usize len = erisdk_cstr_len(txt);

    if (widget == 0) {
        return 0;
    }

    widget->text = erisdk_str_create(a, (char *)txt, len);
    widget->name = erisdk_str_create(a, (char *)txt, len);
    return widget;
}

EriSdkWidget *erisdk_edit_text(EriSdkArena *a, const char *txt) {
    EriSdkWidget *widget = erisdk_widget(a, ERI_WIDGET_TEXTBOX);

    if (widget == 0) {
        return 0;
    }

    widget->text = erisdk_str_create(a, (char *)txt, erisdk_cstr_len(txt));
    return widget;
}

static EriSdkWidget *erisdk_container(EriSdkArena *a, EriWidgetType type, u32 count, va_list args) {
    u32 i = 0;
    EriSdkWidget *widget = erisdk_widget(a, type);

    if (widget == 0) {
        return 0;
    }

    widget->num_children = count;
    widget->children = erisdk_arena_alloc(a, count * sizeof(EriSdkWidget *));
    if (widget->children == 0) {
        return 0;
    }

    for (i = 0; i < count; i++) {
        widget->children[i] = va_arg(args, EriSdkWidget *);
    }

    return widget;
}

EriSdkWidget *erisdk_row(EriSdkArena *a, u32 count, ...) {
    va_list args;
    EriSdkWidget *widget = 0;

    va_start(args, count);
    widget = erisdk_container(a, ERI_WIDGET_ROW, count, args);
    va_end(args);

    return widget;
}

EriSdkWidget *erisdk_column(EriSdkArena *a, u32 count, ...) {
    va_list args;
    EriSdkWidget *widget = 0;

    va_start(args, count);
    widget = erisdk_container(a, ERI_WIDGET_COLUMN, count, args);
    va_end(args);

    return widget;
}

static u32 eri_encode_u32(u8 *buf, usize off, void *user_data) {
    erisdk_mcpy(user_data, &buf[off], sizeof(u32));
    off += sizeof(u32);
    return off;
}

static u32 eri_encode_widget(u8 *buf, usize off, void *user_data) {
    EriSdkWidget *wid = (EriSdkWidget *)user_data;
    u32 i = 0;

    if (wid->background_color > 0) {
        off = erisdk_encode_tlv(buf, off, ERI_WIDGET_ATTR_BG_COLOR, eri_encode_u32, &wid->background_color);
    }
    if (wid->name.ptr != 0) {
        off = erisdk_encode_tlv(buf, off, ERI_WIDGET_ATTR_NAME, erisdk_encode_str, &wid->name);
    }
    if (wid->text.ptr != 0) {
        off = erisdk_encode_tlv(buf, off, ERI_WIDGET_ATTR_TEXT, erisdk_encode_str, &wid->text);
    }
    if (wid->children != 0 && wid->num_children > 0) {
        usize len = 0;
        usize len_off = 0;

        off = encode_type(buf, (u32)off, ERI_WIDGET_ATTR_CHILDREN);
        len_off = off;
        off += sizeof(u32);

        for (i = 0; i < wid->num_children; i++) {
            off = erisdk_encode_tlv(buf, off, wid->children[i]->type, eri_encode_widget, wid->children[i]);
        }

        len = off - len_off - sizeof(u32);
        erisdk_mcpy(&len, &buf[len_off], sizeof(u32));
    }

    return off;
}

static u32 eri_encode_set_tree_msg(u8 *buf, usize off, void *user_data) {
    EriSdkWidget *tree = (EriSdkWidget *)user_data;
    return erisdk_encode_tlv(buf, off, tree->type, eri_encode_widget, tree);
}

void erisdk_set_tree(EriSdkWidget *tree, const char *path) {
    (void)path;

    out_offset = 0;
    out_offset = erisdk_encode_tlv(out_msg_buf, out_offset, ERI_OUT_MSG_SET_TREE, eri_encode_set_tree_msg, tree);
}

void erisdk_register_message_handler(MessageHandlerCallback callback, void *state) {
    message_handler_callback = callback;
    message_handler_callback_state = state;
}

void erisdk_update(void) {
    u8 *in_msg_buf = erisdk_get_in_msg_buf();
    EriTlv tlv;
    usize off = 0;

    while (in_msg_buf[off] != 0) {
        u32 attr_off = 0;
        EriTlv attr_tlv = {0};

        off += erisdk_parse_tlv(&in_msg_buf[off], &tlv);

        switch (tlv.typ) {
            case ERI_IN_MSG_WIDGET_PRESSED: {
                u32 str_off = 0;
                EriMsgWidgetPressed msg;
                msg.name = 0;
                msg.path = 0;

                while (attr_off < tlv.len) {
                    attr_off += erisdk_parse_tlv(&tlv.val[attr_off], &attr_tlv);

                    switch (attr_tlv.typ) {
                        case ERI_MSG_ATTR_NAME:
                            msg.name = &str_buf[str_off];
                            break;
                        case ERI_MSG_ATTR_PATH:
                            msg.path = &str_buf[str_off];
                            break;
                        default:
                            break;
                    }

                    erisdk_mcpy(attr_tlv.val, &str_buf[str_off], attr_tlv.len);
                    str_off += attr_tlv.len;
                    str_buf[str_off] = 0;
                    str_off += 1;
                }

                message_handler_callback(message_handler_callback_state, ERI_IN_MSG_WIDGET_PRESSED, &msg);
                break;
            }
            case ERI_IN_MSG_TEXT_CHANGED: {
                u32 str_off = 0;
                EriMsgTextChanged msg;
                msg.text = 0;
                msg.path = 0;

                while (attr_off < tlv.len) {
                    attr_off += erisdk_parse_tlv(&tlv.val[attr_off], &attr_tlv);

                    switch (attr_tlv.typ) {
                        case ERI_MSG_ATTR_TEXT:
                            msg.text = &str_buf[str_off];
                            break;
                        case ERI_MSG_ATTR_PATH:
                            msg.path = &str_buf[str_off];
                            break;
                        default:
                            break;
                    }

                    erisdk_mcpy(attr_tlv.val, &str_buf[str_off], attr_tlv.len);
                    str_off += attr_tlv.len;
                    str_buf[str_off] = 0;
                    str_off += 1;
                }

                message_handler_callback(message_handler_callback_state, ERI_IN_MSG_TEXT_CHANGED, &msg);
                break;
            }
            default: {
                erisdk_print_fmt("unknown message type: %\n", tlv.typ);
                break;
            }
        }

    }

    in_msg_buf[0] = 0;
}
