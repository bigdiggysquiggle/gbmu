#ifndef GBDEBUGCLASS
#define GBDEBUGCLASS
#include "gb.hpp"
#include <fcntl.h>
#include <unistd.h>

//Exists as an extension of the gb class. Designed this way in
//order to simplify hardware and memory access.
//
//For now in order to use it, run `./gbmu [rom]` followed by
//any combination of the below flags (if desired).
//
//`-o [filename (optional)]`
//		- sends output to the desired file
//		- defaults to "debug_output.txt"
//`-f [default | binjgb]`
//		- specifies what style to print debug information
//		- implemented to allow diffing against other emulators
//		  debug information
//`-d [cpu | mode | state | vram | mmu]`
//		- supports any combination of the above options
//		- specifies what data you want to see in the debug
//		  output
//
//Note: currently vram and mmu are unimplemented options. I'm
//not sure how I want to implement them at this time.

enum flagset {output_file = 1, output_format = 2, output_data = 4};
enum dataflags {cpu_instrs = 1, ppu_mode = 2, ppu_state = 4, ppu_vram = 8, mmu_access = 16};
enum formatflags {default_output, binjgb};

struct s_flags {
	const char *f_string;
	unsigned f_val;
};

struct	s_debugmsg
{
	const char	*str;
	int			args;
};

class debuggerator : public gb {
	public:
		debuggerator();
		~debuggerator();
		debuggerator(sys_type type);
		void	setflags(int ac, char **av);
		void	cpu_print();
		void	debug_msg();
		void	frame_advance();

	private:
		unsigned flags;
		unsigned format_type;
		unsigned output_data;
		int output_file;
};

#endif
