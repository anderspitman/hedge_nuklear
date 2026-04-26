#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <time.h>
#include <dlfcn.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include "nuklear.h"
#include "nuklear_glfw_gl3.h"

#include "eri_sdk.h"

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

typedef struct Context {
    u8 *in_msg_buf;
    usize in_msg_off;
} Context;


static void error_callback(int e, const char *d)
{printf("Error %d: %s\n", e, d);}

static u32 button_press_callback(u8 *buf, usize off, void *user_data) {
    EriSdkWidget *wid = (EriSdkWidget *)user_data;
    off = erisdk_encode_tlv(buf, off, ERI_MSG_ATTR_NAME, erisdk_encode_str, &wid->name);
    return off;
}

static void render_widget(Context *ctx, EriSdkWidget *wid, struct nk_context *nuk_ctx, usize depth) {
    usize i = 0;
    (void)depth;

    switch (wid->type) {
        case ERI_WIDGET_COLUMN: {
            break;
        }
        case ERI_WIDGET_ROW: {
            nk_layout_row_dynamic(nuk_ctx, 60, wid->num_children);
            break;
        }
        case ERI_WIDGET_BUTTON: {
            if (nk_button_label(nuk_ctx, erisdk_str_c(&wid->name))) {
                ctx->in_msg_off = erisdk_encode_tlv(ctx->in_msg_buf, ctx->in_msg_off, ERI_IN_MSG_WIDGET_PRESSED,
                    button_press_callback, wid);
            }
            break;
        }
        case ERI_WIDGET_TEXTBOX: {
            nk_layout_row_dynamic(nuk_ctx, 35, 1);
            nk_edit_string_zero_terminated(nuk_ctx, NK_EDIT_READ_ONLY, erisdk_str_c(&wid->text), (int)erisdk_str_len(&wid->text) + 1, nk_filter_default);
            break;
        }
        default: {
            break;
        }
    }

    for (i = 0; i < wid->num_children; i += 1) {
        EriSdkWidget *child = wid->children[i];
        render_widget(ctx, child, nuk_ctx, depth + 1);
    }
}

static void render_tree(Context *ctx, EriSdkWidget *tree, struct nk_context *nuk_ctx) {
    usize depth = 0;
    if (tree == 0) {
        return;
    }
    render_widget(ctx, tree, nuk_ctx, depth);
}

int main(int argc, char *argv[])
{
    /* Platform */
    struct nk_glfw glfw = {0};
    static GLFWwindow *win;
    int width = 0, height = 0;
    struct nk_context *nuk_ctx;
    struct nk_colorf bg;
    void *app_handle = 0;
    const char *app_path = 0;
    EriInit eri_init = 0;
    EriUpdate erisdk_update = 0;
    EriGetMsgBuf erisdk_get_in_msg_buf = 0;
    EriGetMsgBuf erisdk_get_out_msg_buf = 0;
    u8 *out_msg_buf = 0;
    EriTlv tlv = {0};
    EriSdkWidget *tree = 0;
    u32 off = 0;
    EriSdkArena widget_arena = {0};
    static u8 widget_buf[1*1024*1024];
    Context ctx = {0};
    const char *display_env = 0;
    const char *wayland_env = 0;

    if (argc < 2) {
        printf("Not enough args\n");
        exit(1);
    }

    app_path = argv[1];

    printf("app_path: %s\n", app_path);

    app_handle = dlopen(app_path, RTLD_LAZY);
    if (!app_handle) {
        printf("%s\n", dlerror());
        exit(1);
    }

    *(void **)(&eri_init) = dlsym(app_handle, "eri_init");
    if (!eri_init) {
        printf("%s\n", dlerror());
        exit(1);
    }

    *(void **)(&erisdk_update) = dlsym(app_handle, "erisdk_update");
    if (!erisdk_update) {
        printf("%s\n", dlerror());
        exit(1);
    }

    *(void **)(&erisdk_get_in_msg_buf) = dlsym(app_handle, "erisdk_get_in_msg_buf");
    if (!erisdk_get_in_msg_buf) {
        printf("%s\n", dlerror());
        exit(1);
    }

    *(void **)(&erisdk_get_out_msg_buf) = dlsym(app_handle, "erisdk_get_out_msg_buf");
    if (!erisdk_get_out_msg_buf) {
        printf("%s\n", dlerror());
        exit(1);
    }

    ctx.in_msg_buf = erisdk_get_in_msg_buf();
    ctx.in_msg_off = 0;
    out_msg_buf = erisdk_get_out_msg_buf();

    erisdk_arena_init(&widget_arena, widget_buf, sizeof(widget_buf));

    eri_init();

    off += erisdk_parse_tlv(out_msg_buf, &tlv);
    printf("%d\n", tlv.len);

    if (tlv.typ == ERI_OUT_MSG_SET_TREE) {
        tree = erisdk_parse_tree(&widget_arena, tlv.val, tlv.len);
    }

    /* GLFW */
    glfwSetErrorCallback(error_callback);
    display_env = getenv("DISPLAY");
    wayland_env = getenv("WAYLAND_DISPLAY");
    if ((display_env == 0 || display_env[0] == '\0') &&
        (wayland_env == 0 || wayland_env[0] == '\0')) {
        fprintf(stdout, "[GFLW] no display detected, skipping windowed run\n");
        return tree != 0 ? 0 : 1;
    }
    if (!glfwInit()) {
        fprintf(stdout, "[GFLW] failed to init!\n");
        return tree != 0 ? 0 : 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    win = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Hedge", NULL, NULL);
    if (!win) {
        fprintf(stdout, "[GFLW] failed to create window!\n");
        glfwTerminate();
        return tree != 0 ? 0 : 1;
    }
    glfwMakeContextCurrent(win);
    glfwGetWindowSize(win, &width, &height);

    /* OpenGL */
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glewExperimental = 1;
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to setup GLEW\n");
        exit(1);
    }

    nuk_ctx = nk_glfw3_init(&glfw, win, NK_GLFW3_INSTALL_CALLBACKS);
    {
        struct nk_font_atlas *atlas;
        nk_glfw3_font_stash_begin(&glfw, &atlas);
        nk_glfw3_font_stash_end(&glfw);
    }

    bg.r = 0.10f, bg.g = 0.18f, bg.b = 0.24f, bg.a = 1.0f;
    while (!glfwWindowShouldClose(win))
    {
        /*EriMsgWidgetPressed msg;*/

        /* Input */
        glfwPollEvents();
        nk_glfw3_new_frame(&glfw);

        /* GUI */
        if (nk_begin(nuk_ctx, "Hedge", nk_rect(0, 0, width, height), 0)) {
            render_tree(&ctx, tree, nuk_ctx);
        }
        nk_end(nuk_ctx);

        erisdk_update();

        if (out_msg_buf[0] != 0) {
            off = 0;
            erisdk_arena_reset(&widget_arena, 0);
            off += erisdk_parse_tlv(out_msg_buf, &tlv);
            if (tlv.typ == ERI_OUT_MSG_SET_TREE) {
                tree = erisdk_parse_tree(&widget_arena, tlv.val, tlv.len);
            }
            out_msg_buf[0] = 0;
        }

        ctx.in_msg_buf[0] = 0;
        ctx.in_msg_off = 0;

        /* Draw */
        glfwGetWindowSize(win, &width, &height);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(bg.r, bg.g, bg.b, bg.a);
        nk_glfw3_render(&glfw, NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
        glfwSwapBuffers(win);
    }
    nk_glfw3_shutdown(&glfw);
    glfwTerminate();
    return 0;
}
