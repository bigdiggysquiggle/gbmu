#ifndef PPU
#define PPU
#include "mmu.hpp"
#include <chrono>
#include <iostream>
#include <SDL2/SDL.h>

class ppu {
	public:
		ppu(std::shared_ptr<mmu>);
		int	frameRender(unsigned char);

		unsigned		_cycles;
		inline unsigned char offcheck(unsigned char cyc)
		{
			unsigned char lcdc = (_mmu->PaccessAt(0xFF40) & (1 << 7));
			if (lcdc)
			{
				if (_off == true)
					_off = false;
				_cycles -= cyc;
			}
			else if (_off == false)
			{
				_off = true;
				for (unsigned i = 0; i < 23040; i++)
					pixels[i] = 0xFFFFFFFF;
				_cycles = 70224;
				_mmu->STATupdate(0);
				_mmu->writeTo(0xFF44, 0);
				return 1;
			}
			return 0;
		}
		unsigned	pixels[23040];
		bool			_off;
	private:
		unsigned char	_hblank;
		unsigned char	_pause;
		unsigned		_tot;
//		unsigned char	_lastpause;
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
		unsigned char	_modenxt;

		void	vdump(unsigned short);

		void VBlank();
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
