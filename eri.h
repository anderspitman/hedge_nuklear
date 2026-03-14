#ifndef ERI_H
#define ERI_H

typedef unsigned char u8;
typedef char u8_must_be_1_byte[sizeof(u8) == 1 ? 1 : -1];
typedef int i32;
typedef char i32_must_be_4_bytes[sizeof(i32) == 4 ? 1 : -1];
typedef unsigned int u32;
typedef char u32_must_be_4_bytes[sizeof(u32) == 4 ? 1 : -1];
typedef unsigned long usize;
typedef char usize_must_hold_pointer[sizeof(usize) >= sizeof(void*) ? 1 : -1];

typedef u32 EriType;

typedef u8 *(*EriGetMsgBuf)(void);

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


typedef struct EriTlv {
    u32 typ;
    u32 len;
    u8 *val;
} EriTlv;

#endif /* ERI_H */
