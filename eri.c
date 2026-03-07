#include "eri.h"
#include "common.h"


static u8 in_msg_buf[1*1024*1024];
static u8 out_msg_buf[1*1024*1024];

static usize out_offset = 0;

u8 *eri_get_in_msg_buf(void) {
    return in_msg_buf;
}

u8 *eri_get_out_msg_buf(void) {
    return out_msg_buf;
}

void eri_widget_init(EriWidget *widget, EriWidgetType type) {
    widget->type = type;
    widget->background_color = 0x000000;
    widget->num_children = 0;
    widget->children = NULL;
    widget->text = NULL;
    widget->name = NULL;
}

static u32 encode_type(u8 *buf, u32 off, EriSizedType type) {
    mcpy(&type, &buf[off], sizeof(EriSizedType));
    off += sizeof(EriSizedType);
    return off;
}

/*void eri_parse_msgs(u8 *buf, EriInMessage *msgs) {
    u8 done = 0;

    while (!done) {
        break;
    }
}
*/

u32 eri_parse_tlv(u8 *buf, EriTlv *tlv) {
    u32 off = 0;

    mcpy(&buf[off], &tlv->typ, sizeof(4));
    off += 4;
    mcpy(&buf[off], &tlv->len, sizeof(4));
    off += 4;
    tlv->val = &buf[off];

    return off;
}

u32 encode_tlv(u8 *buf, usize off, EriSizedType type, TlvCallback cb, void *user_data) {

    usize len = 0;
    usize len_off = 0;

    off = encode_type(buf, off, type);

    /* save offset of length and reserve space for it */
    len_off = off;
    off += sizeof(u32);

    off = cb(buf, off, user_data);

    /* write length */
    len = off - len_off - sizeof(u32);
    mcpy(&len, &buf[len_off], sizeof(u32));

    /* TODO: bad idea? */
    buf[off] = 0;

    return off;
}

static u32 encode_u32(u8 *buf, usize off, void *user_data) {

    mcpy(user_data, &buf[off], sizeof(u32));
    off += sizeof(u32);

    return off;
}

static u32 encode_str(u8 *buf, usize off, void *user_data) {

    const char *str = (const char *)user_data;

    u32 len = str_len(str);

    mcpy(str, &buf[off], len);
    off += len;

    return off;
}

static u32 encode_widget(u8 *buf, usize off, void *user_data) {

    EriWidget *wid = (EriWidget *)user_data;

    if (wid->background_color > 0) {
        off = encode_tlv(buf, off, ERI_WIDGET_ATTR_BG_COLOR, encode_u32, &wid->background_color);
    }

    if (wid->name != 0) {
        off = encode_tlv(buf, off, ERI_WIDGET_ATTR_NAME, encode_str, wid->name);
    }

    if (wid->text != 0) {
        off = encode_tlv(buf, off, ERI_WIDGET_ATTR_TEXT, encode_str, wid->text);
    }

    if (wid->children != 0 && wid->num_children > 0) {

        usize len = 0;
        usize len_off = 0;
        u32 i;

        off = encode_type(buf, off, ERI_WIDGET_ATTR_CHILDREN);

        /* save offset of length and reserve space for it */
        len_off = off;
        off += sizeof(u32);
        for (i = 0; i < wid->num_children; ++i) {
            EriWidget *child = wid->children[i];
            off = encode_tlv(buf, off, child->type, encode_widget, child);
        }

        /* write length */
        len = off - len_off - sizeof(u32);
        mcpy(&len, &buf[len_off], sizeof(u32));
    }

    return off;
}

static u32 encode_set_tree_msg(u8 *buf, usize off, void *user_data) {

    EriWidget *tree = (EriWidget *)user_data;
    off = encode_tlv(buf, off, tree->type, encode_widget, tree);

    return off;
}


void eri_set_tree(EriWidget *tree, const char *path) {

    (void)path;

    out_offset = encode_tlv(out_msg_buf, out_offset, ERI_OUT_MSG_SET_TREE, encode_set_tree_msg, tree);
}

MessageHandlerCallback message_handler_callback = NULL;
void *message_handler_callback_state = NULL;

void eri_register_message_handler(MessageHandlerCallback callback, void *state) {
	message_handler_callback = callback;
    message_handler_callback_state = state;
}

void arena_init(Arena *arena, u8 *buf, usize size) {
    arena->offset = 0;
    arena->buf = buf;
    arena->size = size;
}

void *arena_alloc(Arena *arena, usize size) {
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

i32 arena_reset(Arena *arena, usize offset) {
    if (offset >= arena->size) {
        return -1;
    }

    arena->offset = offset;

    return 0;
}

Attr *attr_loop(Arena *a, u32 type, u32 count, va_list args) {
    u32 i;
    Attr *at = arena_alloc(a, sizeof(Attr));
    at->type = type;
    at->num_values = count;
    at->values = arena_alloc(a, count*sizeof(void *));


    for (i = 0; i < count; i++) {
        at->values[i] = va_arg(args, void *);
    }

    return at;
}

Attr *attr(Arena *a, u32 type, u32 count, ...) {
    va_list args;
    Attr *at = 0;
    va_start(args, count);
    at = attr_loop(a, type, count, args);
    va_end(args);
    return at;
}

Attr *name(Arena *a, const char *name) {
    return attr(a, ERI_WIDGET_ATTR_NAME, 1, name);
}

Attr *text(Arena *a, const char *text) {
    return attr(a, ERI_WIDGET_ATTR_TEXT, 1, text);
}

Attr *children(Arena *a, u32 count, ...) {
    va_list args;
    Attr *c = 0;
    va_start(args, count);
    c = attr_loop(a, ERI_WIDGET_ATTR_CHILDREN, count, args);
    va_end(args);
    return c;
}

EriWidget *widget_inner(Arena *a, u32 type, u32 count, va_list args) {
    u32 i = 0;
    EriWidget *w = arena_alloc(a, sizeof(EriWidget));
    Attr *attr = 0;
    w->type = type;

    eri_widget_init(w, type);

    for (i = 0; i < count; i++) {
        attr = va_arg(args, Attr *);

        switch (attr->type) {
            /*case ERI_WIDGET_ATTR_NAME:
                w->name = attr->values[0];
                break;
            */
            case ERI_WIDGET_ATTR_TEXT:
                w->name = attr->values[0];
                w->text = attr->values[0];
                break;
            case ERI_WIDGET_ATTR_CHILDREN:
                w->children = (EriWidget **)attr->values;
                w->num_children = attr->num_values;
                break;
        }
    }

    return w;
}

EriWidget *widget(Arena *a, u32 type, u32 count, ...) {
    va_list args;
    EriWidget *w = 0;
    va_start(args, count);
    w = widget_inner(a, type, count, args);
    va_end(args);
    return w;
}

EriWidget *button(Arena *a, const char *txt) {
    return widget(a, ERI_WIDGET_BUTTON, 1, text(a, txt), name(a, txt));
}

EriWidget *edit_text(Arena *a, const char *txt) {
    return widget(a, ERI_WIDGET_TEXTBOX, 1, text(a, txt));
}

EriWidget *container_inner(Arena *a, u32 type, u32 count, va_list args) {
    return widget(a, type, 1, 
        attr_loop(a, ERI_WIDGET_ATTR_CHILDREN, count, args)
    );
}

EriWidget *row(Arena *a, u32 count, ...) {
    va_list args;
    EriWidget *w = 0;
    va_start(args, count);
    w = container_inner(a, ERI_WIDGET_ROW, count, args);
    va_end(args);
    return w;
}

EriWidget *column(Arena *a, u32 count, ...) {
    va_list args;
    EriWidget *w = 0;
    va_start(args, count);
    w = container_inner(a, ERI_WIDGET_COLUMN, count, args);
    va_end(args);
    return w;
}
