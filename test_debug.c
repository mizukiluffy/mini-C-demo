#include <stdio.h>

int main() {
    int a = 10;
    float b = 3.14f;
    double c = 2.71828;
    char d = 'A';
    int e;

    for (int i = 0; i < 3; i++) {
        a = a + i;
    }

    if (a > 10) {
        int f = 99;
        b = b + 1.0f;
    }

    printf("a=%d b=%.2f c=%.4f d=%c\n", a, b, c, d);
    return 0;
}
