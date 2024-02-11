#include <stdio.h>

int f(int n) {
	puts("Running f()\n");
	return n / 5;
}

int main() {
	int a = 5;
	short b = 100;
	float zf = 100.0f + 2.f;
	return f(a + b);
}
