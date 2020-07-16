#ifndef PPU
#define PPU
#include "mem.hpp"
#include <chrono>
#include <iostream>
#include <SDL2/SDL.h>

class ppu {
	public:
		ppu(std::shared_ptr<mmu>);
		int	frameRender(void);

		unsigned	pixels[23040];
	private:
		unsigned		_cycles;
		unsigned char	_hblank;
		unsigned char	_pause;
//		unsigned char	_lastpause;
		bool			_off;
//		unsigned char	_x;
		unsigned char	_y;
		std::shared_ptr<mmu>	_mmu;
		unsigned char	_sx;
		unsigned char	_sy;
		unsigned char	_wx;
		unsigned char	_wy;
		unsigned char	LCDC;
		unsigned char	STAT;
		unsigned char	tiles[384][8][8];
		unsigned char	spritecount;
		unsigned char	spriteattr[10][4];
		unsigned char	_count;

		void	vdump(unsigned short);

		void VBlank(unsigned char);
		void printSprite(unsigned char sprite[16], unsigned char attr[4], unsigned char lcdc);
		void printSprites16(unsigned char);
		void printSprites8(unsigned char);
		void readOAM(unsigned char);
		void readSprites(unsigned char lcdc);
		void refreshTiles();
		void getTiles9(unsigned short addr, unsigned char x, unsigned char y, unsigned char dis);
		void getTiles8(unsigned short addr, unsigned char x, unsigned char y, unsigned char dis);
		void readTiles(unsigned char lcdc);
		void renderLine(unsigned char lcdc);
};

#endif
