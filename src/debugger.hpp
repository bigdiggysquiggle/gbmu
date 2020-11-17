#ifndef GBDEBUGCLASS
#define GBDEBUGCLASS
#include "gb.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <functional>

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
//
//Planned features: breakpoints, interactive mode, memory and
//vram viewing

enum flagset {output_file = 1, output_format = 2, output_data = 4};
enum dataflags {cpu_instrs = 1, ppu_mode = 2, ppu_state = 4, ppu_vram = 8, mmu_access = 16};
enum formatflags {default_output, binjgb};

struct	s_debugmsg
{
	const char	*str;
	int			args;
};

class debuggerator : public gb {
	public:
		debuggerator()
		{
			flags = 0;
			format_type = formatflags::default_output;
			output_data = 0;
			out_file = 0;
			i = 0;
		}
		debuggerator(sys_type type)
		{
			flags = 0;
			format_type = formatflags::default_output;
			output_data = 0;
			out_file = 0;
			i = 0;
		}
		~debuggerator()
		{
			if (out_file > 1)
				close(out_file);
		}
		void	setflags(int, char **);
		void	cpu_print();
		void	debug_msg();
		void	frame_advance();

	private:
		static int		out_file;
		static unsigned flags;
		static unsigned format_type;
		static unsigned output_data;
		static unsigned i;

		typedef void (*f_func)(int, char **);
		struct s_flags {
			unsigned f_val;
			f_func	f_get;
		};

		static void	setFile(int ac, char **av);
		static void	setFormat(int ac, char **av);
		static void	setData(int ac, char **av);

};

#endif
