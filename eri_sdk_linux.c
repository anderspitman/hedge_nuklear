#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "eri_sdk.h"


void byte_to_braille(unsigned char value, char out[4])
{
    unsigned short braille = 0;
    unsigned int codepoint;

    /* Left column (high nibble) */
    if (value & 0x80) braille |= 0x01;  /* bit 7 → dot 1 */
    if (value & 0x40) braille |= 0x02;  /* bit 6 → dot 2 */
    if (value & 0x20) braille |= 0x04;  /* bit 5 → dot 3 */
    if (value & 0x10) braille |= 0x40;  /* bit 4 → dot 7 */

    /* Right column (low nibble) */
    if (value & 0x08) braille |= 0x08;  /* bit 3 → dot 4 */
    if (value & 0x04) braille |= 0x10;  /* bit 2 → dot 5 */
    if (value & 0x02) braille |= 0x20;  /* bit 1 → dot 6 */
    if (value & 0x01) braille |= 0x80;  /* bit 0 → dot 8 */

    codepoint = 0x2800 + braille;

    /* UTF-8 encode (always 3 bytes for U+28xx) */
    out[0] = (char)(0xE0 | ((codepoint >> 12) & 0x0F));
    out[1] = (char)(0x80 | ((codepoint >> 6)  & 0x3F));
    out[2] = (char)(0x80 | ( codepoint        & 0x3F));
    out[3] = '\0';
}



extern MessageHandlerCallback message_handler_callback;
extern void *message_handler_callback_state;

i32 eri_open(const char *path) {
    i32 fd;
    fd = open(path, O_RDWR | O_CREAT);
    return fd;
}

u32 eri_write(i32 handle, const u8 *bytes, u32 size) {
    return write(handle, bytes, size);
}

static char str_buf[1024];

void eri_update(void) {
    u8 *in_msg_buf = eri_get_in_msg_buf();
    /*
    u32 i = 0;
    */
    EriTlv tlv;
    usize off = 0;

    while (in_msg_buf[off] != 0) {
        u32 attr_off = 0;
        EriTlv attr_tlv = {0};

        /*
        for (i = 0; i < 40; i++) {
            print_fmt("% ", in_msg_buf[i]);
        }
        print("\n");
        */
        

        off += erisdk_parse_tlv(&in_msg_buf[off], &tlv);

        switch (tlv.typ) {
            case ERI_IN_MSG_WIDGET_PRESSED: {

                u32 str_off = 0;

                EriMsgWidgetPressed msg;
                msg.name = 0;
                msg.path = 0;

                while (attr_off < tlv.len) {
                    attr_off += erisdk_parse_tlv(&tlv.val[attr_off], &attr_tlv);
                    attr_off += attr_tlv.len;

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

                    mcpy(attr_tlv.val, &str_buf[str_off], attr_tlv.len);

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
                    attr_off += attr_tlv.len;

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

                    mcpy(attr_tlv.val, &str_buf[str_off], attr_tlv.len);

                    str_off += attr_tlv.len;
                    str_buf[str_off] = 0;
                    str_off += 1;
                }

                message_handler_callback(message_handler_callback_state, ERI_IN_MSG_TEXT_CHANGED, &msg);

                break;
            }
            default: {
                print_fmt("unknown message type: %\n", tlv.typ);
                break;
            }
        }

        off += tlv.len;
    }

    in_msg_buf[0] = 0;
}

#if 0
void sleep_ms(unsigned int ms) {
    struct timespec req, rem;

    req.tv_sec  = ms / 1000;
    req.tv_nsec = (ms % 1000) * 1000000L;

    /* nanosleep can be interrupted by signals, so loop */
    while (nanosleep(&req, &rem) == -1 && errno == EINTR) {
        req = rem;
    }
}

int main(void) {

	/*EriEvent evt;
     */
    usize i = 0;

    u8 *out_msg_buf = eri_get_out_msg_buf();

    char braille_buf[4];

    for (i = 0; i < 1024; i++) {
        out_msg_buf[i] = 0xE7;
    }

    eri_init();

	for (;;) {
        u32 type = 0;
        u32 size = 0;
        memcpy(&type, out_msg_buf, 4);
        memcpy(&size, &out_msg_buf[4], 4);

        printf("Message: { type: %d, size: %d }\n", type, size);

        for (i = 0; i < size; i++) {
            /*if (out_msg_buf[i] >= ' ' && out_msg_buf[i] <= '~') {
                printf("%c", out_msg_buf[i]);
            }
            */
            if (out_msg_buf[i] == 0) {
                /*printf("□");
                 */
                printf("･");
            }
            else {
                byte_to_braille(out_msg_buf[i], braille_buf);
                printf("%s", braille_buf);
            }
        }
        printf("\n");

		if (NULL != message_handler_callback) {
            /*evt.type = ERI_EVENT_TYPE_FRAME;
			message_handler_callback(&evt);
            */
		}
		sleep_ms(16);
	}

	return 0;
}
#endif
