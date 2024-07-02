#include <fcntl.h>
#include <string.h>

void test_open(int fd) {
    assert(fd != 0);
}

void test_write(int fd) {
    write(fd, "12345", 5);
    char buf[2] = {};
    read(fd, buf, 2);
    assert(!strncmp(buf, '12', 5));
}

void test_read(int fd) {
    char buf[3] = {};
    read(fd, buf, 3);
    assert(!strncmp(buf, '345', 5));
}

int main(void) {
    int fd = open('/dev/char',  O_RDWR);
    test_open(fd);
    test_write(fd);
    test_read(fd);
    close(fd);
}
