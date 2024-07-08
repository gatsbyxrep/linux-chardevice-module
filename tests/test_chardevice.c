#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <assert.h>

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


void test_open(int fd) {
    printf("[test_open]: opened fd: %d\n", fd);
    assert(fd != 0);
}

void test_write(int fd) {
    int res = write(fd, "12345", 5);
    printf("[test_write]: write res %d\n", res);
    assert(res == 5);
    char buf[4] = {};
    read(fd, buf, 4);
    printf("[test_write]: read %s\n", buf);
    assert(!strncmp(buf, "1234", 4));
}

void test_read(int fd) {
    char buf[1] = {};
    read(fd, buf, 1);
    printf("[test_read]: read %s\n", buf);
    assert(!strncmp(buf, "5", 1));
}

void test_ioctl(int fd) {
    struct CharDevice_ioctl_last_read lastRead;
    if (ioctl(fd, CharDevice_IOCTL_GET_LAST_READ, &lastRead) < 0) {
        perror("Failed to get last read time");
        return;
    }
    printf("[test_ioctl]: last read by PID %d at time %lld\n", lastRead.pid, lastRead.time / 1000000000LL);
}
int main(void) {
    int fd = open("/dev/char",  O_RDWR);
    test_open(fd);
    test_write(fd);
    test_read(fd);
    test_ioctl(fd);
    close(fd);
}
