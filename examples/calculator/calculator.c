#include "eri_sdk.h"

typedef enum Op {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_NONE
} Op;

typedef struct AppState {
    EriSdkWidget *root_widget;
    EriSdkArena *ui_arena;
    char scratch[256];
    char cur_text[1024];
    usize toff;
    i32 r1;
    i32 r2;
    Op op;
} AppState;

static void init_state(AppState *s) {
    s->root_widget = 0;
    s->ui_arena = 0;
    s->toff = 0;
    s->r1 = 0;
    s->r2 = 0;
    s->op = OP_NONE;

    s->cur_text[0] = '0';
    s->cur_text[1] = '\0';
}


static void render(AppState *state) {

    EriSdkWidget *root;
    EriSdkArena *a = state->ui_arena;

    erisdk_arena_reset(a, 0);

    root = erisdk_column(a, 5,
        erisdk_edit_text(a, state->cur_text),
        erisdk_row(a, 4,
            erisdk_button(a, "7"),
            erisdk_button(a, "8"),
            erisdk_button(a, "9"),
            erisdk_button(a, "/")
        ),
        erisdk_row(a, 4,
            erisdk_button(a, "4"),
            erisdk_button(a, "5"),
            erisdk_button(a, "6"),
            erisdk_button(a, "*")
        ),
        erisdk_row(a, 4,
            erisdk_button(a, "1"),
            erisdk_button(a, "2"),
            erisdk_button(a, "3"),
            erisdk_button(a, "-")
        ),
        erisdk_row(a, 4,
            erisdk_button(a, "0"),
            erisdk_button(a, "."),
            erisdk_button(a, "="),
            erisdk_button(a, "+")
        )
    );

    erisdk_set_tree(root, "/");
}

static char op_to_char(Op op) {
    switch(op) {
        case OP_ADD:
            return '+';
        case OP_SUB:
            return '-';
        case OP_MUL:
            return '*';
        case OP_DIV:
            return '/';
        case OP_NONE:
            return 'N';
    }

    return 'N';
}

static void handle_button(AppState *s, const char *btn_text) {

    i32 *r = 0;
    usize len = 0;
    usize off = 0;

    if (s->op == OP_NONE) {
        r = &s->r1;
    }
    else {
        r = &s->r2;
    }

    if (erisdk_str_eq("0", btn_text)) {
        *r = 10 * *r;
    }
    else if (erisdk_str_eq("1", btn_text)) {
        *r = (10 * *r) + 1;
    }
    else if (erisdk_str_eq("2", btn_text)) {
        *r = (10 * *r) + 2;
    }
    else if (erisdk_str_eq("3", btn_text)) {
        *r = (10 * *r) + 3;
    }
    else if (erisdk_str_eq("4", btn_text)) {
        *r = (10 * *r) + 4;
    }
    else if (erisdk_str_eq("5", btn_text)) {
        *r = (10 * *r) + 5;
    }
    else if (erisdk_str_eq("6", btn_text)) {
        *r = (10 * *r) + 6;
    }
    else if (erisdk_str_eq("7", btn_text)) {
        *r = (10 * *r) + 7;
    }
    else if (erisdk_str_eq("8", btn_text)) {
        *r = (10 * *r) + 8;
    }
    else if (erisdk_str_eq("9", btn_text)) {
        *r = (10 * *r) + 9;
    }
    else if (erisdk_str_eq("+", btn_text)) {
        s->op = OP_ADD;
    }
    else if (erisdk_str_eq("-", btn_text)) {
        s->op = OP_SUB;
    }
    else if (erisdk_str_eq("*", btn_text)) {
        s->op = OP_MUL;
    }
    else if (erisdk_str_eq("/", btn_text)) {
        s->op = OP_DIV;
    }
    else if (erisdk_str_eq("=", btn_text)) {
        if (s->r2 == 0) {
            s->r1 = 0;
        }

        switch (s->op) {
            case OP_ADD:
                s->r1 = s->r1 + s->r2;
                break;
            case OP_SUB:
                s->r1 = s->r1 - s->r2;
                break;
            case OP_MUL:
                s->r1 = s->r1 * s->r2;
                break;
            case OP_DIV:
                s->r1 = s->r1 / s->r2;
                break;
            case OP_NONE:
                break;
        }
        s->r2 = 0;
        s->op = OP_NONE;
    }

    erisdk_fmts(s->scratch, "%", s->r1);
    len = erisdk_cstr_len(s->scratch);
    erisdk_mcpy(s->scratch, s->cur_text, len);
    off += len;

    if (s->op != OP_NONE && s->r2 != 0) {
        s->cur_text[off++] = op_to_char(s->op);

        erisdk_fmts(s->scratch, "%", s->r2);
        len = erisdk_cstr_len(s->scratch);
        erisdk_mcpy(s->scratch, &s->cur_text[off], len);
        off += len;
    }

    s->cur_text[off++] = '\0';
}

static void message_handler(void *state_in, u32 type, void *msg) {

    AppState *s = 0;

    s = (AppState *)state_in;


    switch (type) {
        case ERI_IN_MSG_WIDGET_PRESSED: {
            EriMsgWidgetPressed *pressed_msg = (EriMsgWidgetPressed *)msg;

            handle_button(s, pressed_msg->name);

            erisdk_print(s->cur_text);
            erisdk_print("\n");

            break;
        }
        default:
            break;
    }

    render(s);
}


void eri_init(void) {

    static AppState state;
    static EriSdkArena ui_arena;
    static u8 buf[1*1024*1024];

    init_state(&state);

    state.ui_arena = &ui_arena;

    erisdk_arena_init(&ui_arena, buf, sizeof(buf));

    render(&state);

    erisdk_register_message_handler(message_handler, &state);
}
