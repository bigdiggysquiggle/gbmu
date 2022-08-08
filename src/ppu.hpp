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
		ppu(std::shared_ptr<mmu>, uint8_t);
		void		cycle();
		uint32_t	pixels[23040];
		bool		_off;

	private:
		uint32_t _cycles;
		uint32_t _oncyc;
		uint32_t _dclk;
		uint32_t _sclk;

		uint8_t _x;
		uint8_t _y;

		uint8_t sx;
		uint8_t sy;
		uint8_t wx;
		uint8_t wy;
		uint8_t lcdc;

		uint8_t cstate;
		std::shared_ptr<mmu> _mmu;
		uint8_t spritecount;
		uint8_t spriteindex;
		uint8_t spriteattr[10][4];
		laz_e	sptile;
//		uint8_t sptile[8];
		bool		  sprite;

		uint8_t tilenum;
		bool	iswin;
		uint8_t tbyte[2];
		laz_e	bwtile;
//		uint8_t bwtile[22][8];
		uint8_t ctile;

		uint32_t ctab[4];
		uint32_t bgp[4];
		uint32_t obp0[4];
		uint32_t obp1[4];

		void	_cycle(bool);
		void	drawpix();
		void	_drawpix(bool);
		void	_spritebranch();
		void	gettnum();
		void	spritecheck();
		void	getsprite();
		void	getbyte();
//		void	genBuf(uint8_t *);
		void	genBuf(laz_e);
		void	fetch8(uint8_t);
		void	fetch9(uint8_t);
		void	palletcalc();
		inline uint8_t offcheck(uint8_t cyc)
		{
			uint8_t lcdon = (_mmu->PaccessAt(0xFF40) & (1 << 7));
			if (lcdon)
			{
				PRINT_DEBUG("lcdon");
				if (_off == true)
					_off = false;
				if (_oncyc)
				{
					_oncyc = (cyc > _oncyc) ? 0 : _oncyc - cyc;
					PRINT_DEBUG("_oncyc %u", _oncyc);
					if (!_oncyc)
					{
						PRINT_DEBUG("statup");
						palletcalc();
						_mmu->STATupdate(2);
					}
					return 1;
				}
			}
			else
			{
				PRINT_DEBUG("lcdoff");
				if (_off == false)
				{
					_off = true;
					for (uint32_t i = 0; i < 23040; i++)
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
		inline uint8_t	getMode()
		{
			return  (_mmu->PaccessAt(0xFF41) & 3);
		}
		inline uint8_t	getState()
		{
			return (cstate);
		}
		friend class debuggerator;
};

#endif
