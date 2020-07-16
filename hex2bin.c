#include <stdio.h>
#define X(x) ((x) >> 6)
#define Y(x) (((x) >> 3) & 7)
#define Z(x) ((x) & 7)
#define P(x) (((x) >> 4) & 3)
#define Q(x) (((x) >> 3) & 1)

int		htoi(char *s)
{
	char *st = "0123456789abcdef";
	int i;
	int tot = 0;
	while (*s)
	{
		i = 0;
		while (st[i] && st[i] != *s)
			i++;
		tot *= 16;
		tot += i;
		s++;
	}
	return tot;
}

void	printbin(unsigned char c)
{
	unsigned char i = 1 << 7;
	while (i)
	{
		printf("%d", i & c ? 1 : 0);
		i >>= 1;
	}
	printf(" X %d, Y %d, Z %d, P %d, Q %d\n", X(c), Y(c), Z(c), P(c), Q(c));
}

int main(int ac, char **av)
{
	for (int i = 1; i < ac; i++)
		printbin(htoi(av[i]));
}
