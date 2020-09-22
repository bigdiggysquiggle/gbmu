#include "gb.hpp"
#include <SDL2/SDL.h>

#define WIN_WIDTH 160
#define WIN_HEIGHT 144
#define CPU_FREQ 4194304
#define FRAME_TIME 70224


gb::gb()
{
	_cycles = 0;
	gb(dmg);
}

gb::gb(sys_type type)
{
	_cycles = 0;
	_mmu = std::make_shared<mmu>(type);
	_ppu = std::make_shared<ppu>(_mmu, type);
	_cpu = std::make_unique<cpu>(_mmu, _ppu);
}

void	gb::load_cart(char *name)
{
	_mmu->loadCart(name);
	_cpu->reset();
}

void	gb::frame_advance(SDL_Texture *frame)
{
	unsigned framecount = 0;
	unsigned cyc;
	while (framecount < FRAME_TIME)
	{
		_mmu->pollInput();
		cyc = _cpu->opcode_parse();
		_mmu->timerInc(cyc);
		_cycles += cyc;
		if (_cycles >= CPU_FREQ)
			_cycles -= CPU_FREQ;
	}
	SDL_UpdateTexture(frame, NULL, _ppu->pixels, (160 * 4));
}
