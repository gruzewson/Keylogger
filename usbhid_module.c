#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/input.h> // defines standard keycodes
#include <linux/hid.h>
#include <linux/keyboard.h>
#include <linux/uaccess.h>

static struct timer_list hello_timer;

// static void hello_timer_fn(struct timer_list *t) {
//     pr_info("Hello\n");
// }

static int keyboard_event(struct notifier_block *nb, unsigned long code, void *param)
{
    struct keyboard_notifier_param *kp = param;

    if (kp->value == KEY_F1 && kp->down) {
        int keycode = (int)kp->value;
        // Start a timer that will print "Hello"
        // timer_setup(&hello_timer, hello_timer_fn, 0);
        // mod_timer(&hello_timer, jiffies + msecs_to_jiffies(500));
        pr_info("Hello %d", keycode);

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
    del_timer_sync(&hello_timer);

    pr_info("Keyboard macro module unloaded\n");
}

module_init(keyboard_macro_init);
module_exit(keyboard_macro_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A simple keyboard macro module");