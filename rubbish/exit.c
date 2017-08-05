#include <stdio.h>

void print()
{
}

int main()
{
	char path[1000];
	char this[100];
	printf("---\n");
	sscanf("hello world", "%255s %255s", path, this);
	printf("%s %shlll\n", path, this);
}
