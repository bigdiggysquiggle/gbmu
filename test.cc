#include "debugger.hpp"

int main(int ac, char **av)
{
	if (ac == 1)
		return 0;
	debuggerator debug;
	debug.setflags(ac - 1, av + 1);
	return 0;
}
