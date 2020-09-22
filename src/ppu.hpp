#ifndef PPU
#define PPU
#include "mmu.hpp"
#include <chrono>
#include <iostream>
#include <SDL2/SDL.h>
#include <stdlib.h>

#define MODE_0 456
#define MODE_1 456
#define MODE_2 80
#define MODE_3 168

#define DMG 1
#define GBC 2

class ppu {
	public:
		ppu(std::shared_ptr<mmu>, unsigned char);
		void	cycle();
		unsigned pixels[23040];
		bool	_off;

	private:
		unsigned _cycles;
		unsigned _spritecyc;
		unsigned _delaycyc;
		bool	sprite;
		unsigned char _x;
		unsigned char _y;
		unsigned char sx;
		unsigned char sy;
		unsigned char wx;
		unsigned char wy;
		unsigned char lcdc;
		unsigned char tilenum;
		std::shared_ptr<mmu> _mmu;
		unsigned char spritecount;
		unsigned char spriteindex;
		unsigned char spriteattr[10][4];
		unsigned char tbyte[2];
		unsigned char tile[8];
		unsigned char stile[8];
		unsigned ctab[4] = {0x00FFFFFF, 0x00AAAAAA, 0x00555555, 0x00000000};
		unsigned bgp[4];
		unsigned obp0[4];
		unsigned obp1[4];

		void	_cycle(bool);
		void	drawpix();
		void	gettile();
		void	spritecheck();
		void	getsprite(unsigned char);
		void	fetch8(unsigned char, unsigned char *);
		void	fetch9(unsigned char, unsigned char *);
		void	palletcalc();
};

#endif
