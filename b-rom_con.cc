#include <cstdio>

int main(int ac, char **av)
{
	if (ac != 2)
	{
		printf("Usage: ./b-rom_con \"filename\"\n");
		return 0;
	}
	unsigned char bootrom[0x100];
	FILE *rom = fopen(av[1], "r");
	fread(bootrom, 1, 256, rom);
	fclose(rom);
	printf("unsigned char _booter[] = {\n\t");
	for (int i = 0; i < 256; i++)
		printf("0x%02hhx,%s", bootrom[i], (i && !((i + 1) % 9)) ? "\n\t" : " ");
	printf("%#04hhx};", bootrom[255]);
}
