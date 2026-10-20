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

printf("\n[调试] 暂停在 第19 行，按 Enter 继续...\n"); getchar();
fprintf(stderr, "[DBG] BP=19 int a=%d\n", a);
fflush(stderr);
fprintf(stderr, "[DBG] BP=19 float b=%f\n", b);
fflush(stderr);
fprintf(stderr, "[DBG] BP=19 double c=%f\n", c);
fflush(stderr);
fprintf(stderr, "[DBG] BP=19 char d=%d\n", d);
fflush(stderr);
fprintf(stderr, "[DBG] BP=19 int e=__NA__\n");
fflush(stderr);
    printf("a=%d b=%.2f c=%.4f d=%c\n", a, b, c, d);
    return 0;
}

