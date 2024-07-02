#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>

#define BUF_SIZE 1024

struct CharDevice {
    char buffer[BUF_SIZE];
    struct mutex mutex; // Struct guard
    int head;
    int tail;
    int blocking;
};

static struct CharDevice chDevice;
static dev_t devNum;
static struct cdev cdev;

static int CharDevice_open(struct inode* inode, struct file* file) {
    file->private_data = &chDevice;
    return 0;
}

static int CharDevice_release(struct inode* inode, struct file* file) {
    return 0;
}

static ssize_t CharDevice_read(struct file* file,
                               char __user* buf,
                               size_t count,
                               loff_t* offset) {

    struct CharDevice* dev = file->private_data;
    ssize_t ret = 0;

    mutex_lock(&dev->mutex);
    if(copy_to_user(buf, dev->buffer, count)) {
        ret = -EFAULT;
        goto out;
    }
out:
    mutex_unlock(&dev->mutex);
    return ret;
}

static ssize_t CharDevice_write(struct file* file,
                                const char __user* buf,
                                size_t count,
                                loff_t* offset) {
    struct CharDevice* dev = file->private_data;
    ssize_t ret = 0;

    mutex_lock(&dev->mutex);
    if(copy_from_user(dev->buffer, buf, count)) {
        ret = -EFAULT;
        goto out;
    }
out:
    mutex_unlock(&dev->mutex);
    return ret;
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
    int ret = alloc_chrdev_region(&devNum, 0, 1, "char");
    if(ret < 0) {
        printk(KERN_ERR "Failed to register character device: %d\n", ret);
        return ret;
    }

    cdev_init(&cdev, &fOps);
    ret = cdev_add(&cdev, devNum, 1);
    if(ret < 0) {
        printk(KERN_ERR "Failed to add character device: %d\n", ret);
        unregister_chrdev_region(devNum, 1);
        return ret;
    }

    memset(chDevice.buffer, 0, BUF_SIZE);
    mutex_init(&chDevice.mutex);
    chDevice.head = 0;
    chDevice.tail = 0;
    chDevice.blocking = 1;
    return 0;
}

static void __exit CharDevice_exit(void) {
    cdev_del(&cdev);
    unregister_chrdev_region(devNum, 1);

    printk(KERN_INFO "Character device driver removed\n");
}

module_init(CharDevice_init);
module_exit(CharDevice_exit);
MODULE_LICENSE("GPL");
module_param(BUF_SIZE, int, 0);
MODULE_PARM_DESC(BUF_SIZE, "Buffer size for the circular buffer");
