#ifndef GB_HPP
#define GB_HPP
#include "cpu.hpp"
#include "mmu.hpp"
#include "ppu.hpp"

#define WIN_WIDTH 160
#define WIN_HEIGHT 144
#define CPU_FREQ 4194304
#define FRAME_TIME 70224

enum sys_type {dmg = 1, gbc = 2};

class gb {
	public:
		gb();
		gb(sys_type);
		void	load_cart(char *);
		virtual void	frame_advance();

		std::unique_ptr<cpu> _cpu;
		std::shared_ptr<mmu> _mmu;
		std::shared_ptr<ppu> _ppu;

	protected:
		unsigned long long	_cycles;
		unsigned			cyc;
};

#endif
