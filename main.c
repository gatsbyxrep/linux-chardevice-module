#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>

static dev_t devNum;
static struct cdev cdev;

static int CharDevice_open(struct inode* inode, struct file* file) {
    return 0;
}

static int CharDevice_release(struct inode* inode, struct file* file) {
    return 0;
}

static ssize_t CharDevice_read(struct file* file,
                               char __user* buf,
                               size_t count,
                               loff_t* offset) {
    return 0;
}

static ssize_t CharDevice_write(struct file* file,
                                const char __user* buf,
                                size_t count,
                                loff_t* offset) {
    return 0;
}

static long CharDevice_ioctl(struct file* file, unsigned int cmd, unsigned long arg) {
    return 0L;
}

static struct file_operations fOps = {
    .owner = THIS_MODULE,
    .open = CharDevice_open,
    .release = CharDevice_release,
    .read = CharDevice_read,
    .write = CharDevice_write,
    .unlocked_ioctl = CharDevice_ioctl,
};

static int __init CharDevice_init(void) {
    return 0;
}

static void __exit CharDevice_exit(void) {

}

module_init(CharDevice_init);
module_exit(CharDevice_exit);
MODULE_LICENSE("GPL");
