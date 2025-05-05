#include <sys/syscall.h>
#include <unistd.h>

int main() {
    const char hw[] = "Hello world\n";
    syscall(SYS_write, 1, hw, 12);
    return 0;
}

