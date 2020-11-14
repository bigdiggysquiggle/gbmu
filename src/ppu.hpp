#ifndef PPU
#define PPU
#include "print_debug.hpp"
#include "mmu.hpp"
#include "lazy.hpp"
#include <chrono>
#include <iostream>
#include <SDL2/SDL.h>
#include <stdlib.h>

#define DMG 1
#define GBC 2

class ppu {
	public:
		ppu(std::shared_ptr<mmu>, unsigned char);
		void	cycle();
		unsigned pixels[23040];
		bool	_off;

	private:
		inline unsigned char offcheck(unsigned char cyc)
		{
			unsigned char lcdon = (_mmu->PaccessAt(0xFF40) & (1 << 7));
			if (lcdon)
			{
//				PRINT_DEBUG("lcdon");
				if (_off == true)
					_off = false;
				if (_oncyc)
				{
					_oncyc = (cyc > _oncyc) ? 0 : _oncyc - cyc;
//					PRINT_DEBUG("_oncyc %u", _oncyc);
					if (!_oncyc)
					{
//						PRINT_DEBUG("statup");
						palletcalc();
						_mmu->STATupdate(2);
					}
					return 1;
				}
			}
			else
			{
//				PRINT_DEBUG("lcdoff");
				if (_off == false)
				{
					_off = true;
					for (unsigned i = 0; i < 23040; i++)
						pixels[i] = 0xFFFFFFFF;
					_oncyc = 70224;
					_cycles = 0;
					spritecount = 0;
					_dclk = 6;
					_y = 0;
					_x = 0;
					cstate = 0;
					sprite = false;
					_mmu->STATupdate(0);
					_mmu->writeTo(0xFF44, 0);
				}
				return 1;
			}
			return 0;
		}
		inline unsigned char	getMode()
		{
			return  (_mmu->PaccessAt(0xFF41) & 3);
		}
		inline unsigned char	getState()
		{
			return (cstate);
		}
		unsigned _cycles;
		unsigned _oncyc;
		unsigned _dclk;
		unsigned _sclk;

		unsigned char _x;
		unsigned char _y;

		unsigned char sx;
		unsigned char sy;
		unsigned char wx;
		unsigned char wy;
		unsigned char lcdc;

		unsigned char cstate;
		std::shared_ptr<mmu> _mmu;
		unsigned char spritecount;
		unsigned char spriteindex;
		unsigned char spriteattr[10][4];
		laz_e		  sptile;
//		unsigned char sptile[8];
		bool		  sprite;

		unsigned char tilenum;
		bool	iswin;
		unsigned char tbyte[2];
		laz_e		  bwtile;
//		unsigned char bwtile[22][8];
		unsigned char ctile;

		unsigned ctab[4];
		unsigned bgp[4];
		unsigned obp0[4];
		unsigned obp1[4];

		void	_cycle(bool);
		void	drawpix();
		void	_drawpix(bool);
		void	_spritebranch();
		void	gettnum();
		void	spritecheck();
		void	getsprite();
		void	getbyte();
//		void	genBuf(unsigned char *);
		void	genBuf(laz_e);
		void	fetch8(unsigned char);
		void	fetch9(unsigned char);
		void	palletcalc();
		friend class debuggerator;
};

#endif
