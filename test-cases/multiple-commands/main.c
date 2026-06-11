#include <stdio.h>

void use_length_macro(void) {
	int len = LENGTH;
	printf("length=%d\n", len);
}

int main(int argc, char** argv) {
	use_length_macro();
	return 0;
}
