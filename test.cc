#include "lazy.hpp"
#include <stdio.h>

int main(void)
{
	try {
	laz_e FUCK(8);
	for (unsigned char i = 0; i < 8; i++)
	{
		FUCK[i] = i % 4;
		printf("%u\n", FUCK[i]);
	}
	for (unsigned char i = 0; i < 8; i++)
		printf("%u ", FUCK.getPix());
	printf("\n");
	for (unsigned char i = 0; i < 8; i++)
		printf("%u ", FUCK[i]);
	printf("\n");
	FUCK.getPix();}
	catch (const char *e)
	{printf("%s\n", e);}
}
