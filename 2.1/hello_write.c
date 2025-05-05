#include <unistd.h>

int main() {
    const char hw[]="Hello world\n";
    write(1, hw, 12);
    return 0;
}


