#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/time.h>
#include <linux/ktime.h>
#include <linux/timekeeping.h>
#include <linux/uaccess.h>
#include <linux/wait.h>

#define COMPAT_MODE

struct CharDevice_ioctl_last_read {
    pid_t pid;
    long long time;
};

struct CharDevice_ioctl_last_write {
    pid_t pid;
    long long time;
};

#define CharDevice_IOCTL_MAGIC 'm'
#define CharDevice_IOCTL_SET_BLOCKING _IOW(CharDevice_IOCTL_MAGIC, 0, int)
#define CharDevice_IOCTL_GET_LAST_READ _IOR(CharDevice_IOCTL_MAGIC, 1, struct CharDevice_ioctl_last_read)
#define CharDevice_IOCTL_GET_LAST_WRITE _IOR(CharDevice_IOCTL_MAGIC, 2, struct CharDevice_ioctl_last_write)

static int BUF_SIZE = 1024;

struct CharDevice {
    char* buffer;
    int head;
    int tail;
    struct mutex mutex; // Struct guard
    wait_queue_head_t readQueue;
    wait_queue_head_t writeQueue;
    int blocking;
    struct timespec64 lastRead;
    struct timespec64 lastWrite;
    pid_t lastReadPid;
    pid_t lastWritePid;
};

static struct CharDevice chDevice;
static dev_t devNum;
static struct cdev cdev;

static int CharDevice_open(struct inode* inode, struct file* file) {
    if(!file) {
        printk(KERN_ERR "Failed to open char device\n");
        return -1;
    }
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

    if(dev->blocking && (dev->head == dev->tail)) {
        wait_event_interruptible(dev->readQueue, dev->head != dev->tail);
    }

    int available = (dev->head >= dev->tail) ? (dev->head - dev->tail)
                                             : (BUF_SIZE - dev->tail + dev->head);
    printk(KERN_DEBUG "Available memory %d\n", available);
    if(available == 0) {
        ret = 0;
        goto out;
    }

    if(count > available) {
        count = available;
    }

    if(copy_to_user(buf, dev->buffer + dev->tail, count)) {
        ret = -EFAULT;
        goto out;
    }

    dev->tail = (dev->tail + count) % BUF_SIZE;
    ktime_get_real_ts64(&dev->lastRead);
    dev->lastReadPid = current->pid;

    wake_up_interruptible(&dev->writeQueue);

    ret = count;

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

    if(dev->blocking && (dev->head == (dev->tail + 1) % BUF_SIZE)) {
        wait_event_interruptible(dev->writeQueue,
                                 dev->head != (dev->tail + 1) % BUF_SIZE);
    }
    int free;
    if (dev->head <= dev->tail) {
        free = BUF_SIZE - dev->tail + dev->head - 1;
    } else {
        free = dev->head - dev->tail - 1;
    }
    if(free == 0) {
        // No space for new data
        printk(KERN_DEBUG "No memory in buffer for write\n");
        ret = 0;
        goto out;
    }

    if(count > free) {
        count = free;
    }

    if(copy_from_user(dev->buffer + dev->head, buf, count)) {
        ret = -EFAULT;
        printk(KERN_WARNING "Cannot copy to user\n");
        goto out;
    }

    dev->head = (dev->head + count) % BUF_SIZE;
    ktime_get_real_ts64(&dev->lastWrite);
    dev->lastWritePid = current->pid;
    // Notify readers
    wake_up_interruptible(&dev->readQueue);

    ret = count;

out:
    mutex_unlock(&dev->mutex);
    return ret;
}

static long CharDevice_ioctl(struct file* file, unsigned int cmd, unsigned long arg) {
    struct CharDevice* dev = file->private_data;
    int ret = 0;

    mutex_lock(&dev->mutex);

    switch (cmd) {
        case CharDevice_IOCTL_SET_BLOCKING: {
            dev->blocking = arg;
            break;
        }
        case CharDevice_IOCTL_GET_LAST_READ: {
            struct CharDevice_ioctl_last_read lastRead;
            lastRead.pid = dev->lastReadPid;
            lastRead.time =
                dev->lastRead.tv_sec * 1000000000LL + dev->lastRead.tv_nsec;
            if(copy_to_user((void __user*)arg, &lastRead, sizeof(lastRead))) {
                ret = -EFAULT;
            }
            break;
        }
        case CharDevice_IOCTL_GET_LAST_WRITE: {
            struct CharDevice_ioctl_last_write lastWrite;
            lastWrite.pid = dev->lastWritePid;
            lastWrite.time =
                dev->lastWrite.tv_sec * 1000000000LL + dev->lastWrite.tv_nsec;
            if(copy_to_user((void __user*)arg, &lastWrite, sizeof(lastWrite))) {
                ret = -EFAULT;
            }
            break;
        }
        default: {
            ret = -ENOIOCTLCMD;
        }
    }

    mutex_unlock(&dev->mutex);
    return ret;
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

    chDevice.buffer = kmalloc(BUF_SIZE * sizeof(char), GFP_KERNEL);
    if(!chDevice.buffer) {
        printk(KERN_ERR "Failed to add character device\n");
        unregister_chrdev_region(devNum, 1);
        return -ENOMEM;
    }
    memset(chDevice.buffer, 0, BUF_SIZE);
    chDevice.head = 0;
    chDevice.tail = 0;
    mutex_init(&chDevice.mutex);
    init_waitqueue_head(&chDevice.readQueue);
    init_waitqueue_head(&chDevice.writeQueue);
    chDevice.blocking = 1;

    printk(KERN_INFO "My character device driver initialized\n");
    return 0;
}

static void __exit CharDevice_exit(void) {
    kfree(chDevice.buffer);
    cdev_del(&cdev);
    unregister_chrdev_region(devNum, 1);

    printk(KERN_INFO "My character device driver removed\n");
}

module_init(CharDevice_init);
module_exit(CharDevice_exit);
MODULE_LICENSE("GPL");
module_param(BUF_SIZE, int, 0);
MODULE_PARM_DESC(BUF_SIZE, "Buffer size for the circular buffer");
