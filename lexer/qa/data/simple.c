#include <stdio.h>

int main(int argc, char** argv) {
    int i = 1000;
    i++; ++i;
    i--; --i;
    i >>= 1;
    i <<= 2;
    i += 1; i -= 1;
    if (i == 1 || i != 2)
        return 0;
    if (i >= 4 && i <= 8)
        return 1;
    const char* str = "this is a string//literal += 100||";
}