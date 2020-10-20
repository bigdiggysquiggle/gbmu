#include "lazy.hpp"
#include <stdio.h>

int main(void)
{
	laz_e FUCK(8);
	for (unsigned char i = 0; i < 2; i++)
		for (unsigned char j = 0; j < 4; j++)
			FUCK.addPix(j);
	for (unsigned char i = 0; i < 8; i++)
		printf("%u ", FUCK.getPix());
	printf("\n");
	for (unsigned char i = 0; i < 8; i++)
		printf("%u ", FUCK[i]);
	printf("\n");
}
