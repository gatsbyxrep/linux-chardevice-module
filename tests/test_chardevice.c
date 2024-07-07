#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

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

int main(void) {
    int fd = open("/dev/char",  O_RDWR);
    test_open(fd);
    test_write(fd);
    test_read(fd);
    close(fd);
}
