#include <stdio.h>

void	print(void *test)
{
	unsigned char *byte = (unsigned char *)test;
	for (int i = 0; i < 4; i++)
		printf("%02x\n", byte[i]);
}

int main(void)
{
	unsigned test = 0xAABBCCFF;
	print(&test);
	return 0;
}
