#include <arch/reg.h>
#include <arch/idt.h>
#include <dev/utils/video.h>
#include <sched/task.h>
#include <dev/dev.h>
#include <dev/tty.h>
#include <dev/utils/keyboard.h>
#include <ipc/ipc.h>
#include <utils/kprint.h>
#include <lib/time.h>
#include <lib/string.h>
#include <lib/stdtypes.h>
#include <lib/assert.h>
#include <lib/stdio.h>
#include <lib/data/clist.h>
#include <messages.h>

static void tty_write(struct io_buf_t *io_buf, void *data, u_int size);
static void tty_read(struct io_buf_t *io_buf, void *buffer, u_int size);
static void tty_ioctl(struct io_buf_t *io_buf, int command);
static void tty_write_ch(char ch);
static char tty_read_ch();

/*
 * Data
 */
static char const tty_output_buff[VIDEO_SCREEN_SIZE]; /* teletype output buffer */
static char const tty_input_buff[VIDEO_SCREEN_WIDTH]; /* teletype input buffer */
const char *tty_dev_name = "tty";                     /* teletype device name */

/*
 * Api - Teletype init
 */
extern void tty_init()
{
    struct clist_head_t *entry;
    struct dev_t dev;
    struct ih_low_t *ih_low;

    /* register teletype device */
    strcpy(dev.name, tty_dev_name);
    dev.base_r = tty_input_buff;
    dev.base_w = tty_output_buff;
    dev.read_cb = tty_read;
    dev.write_cb = tty_write;
    dev.ioctl_cb = tty_ioctl;
    dev.ih_list.head = null;

    /* add interrupt handlers */
    dev.ih_list.slot_size = sizeof(struct ih_low_t);
    entry = clist_insert_entry_after(&dev.ih_list, dev.ih_list.head);
    ih_low = (struct clist_head_t *)entry->data;
    ih_low->number = INT_KEYBOARD;
    ih_low->handler = tty_keyboard_ih_low;

    dev_register(&dev);
}

/*
 * Api - Key pressed
 */
extern void tty_keyboard_ih_low(int number, struct ih_low_data_t *data)
{
    char *keycode = data->data;
    char ch = keyboard_map[*keycode];
    // ???
}

/*
 * Write to tty
 */
static void tty_write(struct io_buf_t *io_buf, void *data, u_int size)
{
    char *ptr = data;

    for (int i = 0; i < size && !io_buf->is_eof; ++i)
    {
        tty_write_ch(io_buf, *ptr++);
    }
}

/*
 * Read from tty
 */
static void tty_read(struct io_buf_t *io_buf, void *buffer, u_int size)
{
    char *ptr = buffer;

    for (int i = 0; i < size && !io_buf->is_eof; ++i)
    {
        char ch = tty_read_ch();
        *ptr = ch;
        // ???
    }
}

/*
 * Teletype specific command
 */
static void tty_ioctl(struct io_buf_t *io_buf, int command)
{
    char *hello_msg = MSG_KERNEL_NAME;

    switch (command)
    {
    case IOCTL_INIT:  /* prepare video device */
        kmode(false); /* detach syslog from screen */
        io_buf->ptr = video_clear(io_buf->base);
        tty_write(io_buf, hello_msg, strlen(hello_msg));
        video_flush(io_buf->base);
        break;
    case IOCTL_CLEAR: /* fill buffer with spaces */
        io_buf->ptr = video_clear(io_buf->base);
        break;
    case IOCTL_FLUSH: /* flush buffer to screen */
        video_flush(io_buf->base);
        break;
    default:
        unreachable();
    }
}

/*
 * Write single character to tty
 */
static void tty_write_ch(struct io_buf_t *io_buf, char ch)
{
    if ((size_t)io_buf->ptr - (size_t)io_buf->base + 1 < VIDEO_SCREEN_SIZE)
    {
        if (ch != '\n')
        {
            /* regular character */
            *io_buf->ptr++ = ch;
        }
        else
        {
            /* new line character */
            int line_pos = ((size_t)io_buf->ptr - (size_t)io_buf->base) % VIDEO_SCREEN_WIDTH;
            for (int j = 0; j < VIDEO_SCREEN_WIDTH - line_pos; ++j)
            {
                *io_buf->ptr++ = ' ';
            }
        }
    }
    else
    {
        io_buf->ptr = video_scroll(io_buf->base, io_buf->ptr);
        tty_write_ch(io_buf, ch);
    }
}

/*
 * Read single character from tty
 */
static char tty_read_ch()
{
}