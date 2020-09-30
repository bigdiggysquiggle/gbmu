#ifndef GB_HPP
#define GB_HPP
#include "cpu.hpp"
#include "mmu.hpp"
#include "ppu.hpp"

enum sys_type {dmg = 1, gbc = 2};

class gb {
	public:
		gb();
		gb(sys_type);
		void	load_cart(char *);
		void	frame_advance();

		std::unique_ptr<cpu> _cpu;
		std::shared_ptr<mmu> _mmu;
		std::shared_ptr<ppu> _ppu;

	protected:
		unsigned	_cycles;
};

#endif
