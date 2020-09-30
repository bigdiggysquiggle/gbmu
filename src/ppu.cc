#include "ppu.hpp"
#define LCDC 0xFF40
#define STAT 0xFF41
#define SCY 0xFF42
#define SCX 0xFF43
#define LY 0xFF44
#define BGP 0xFF47
#define OBP0 0xFF48
#define OBP1 0xFF49
#define WY 0xFF4A
#define WX 0xFF4B
#define SPRITE_Y 0
#define SPRITE_X 1
#define SPRITE_T 2
#define SPRITE_A 3

ppu::ppu(std::shared_ptr<mmu> mem, unsigned char type) : _mmu(mem)
{
	for (unsigned i; i < 23040; i++)
		pixels[i] = 0x00FFFFFF;
	_x = 0;
	_y = 0;
	spritecount = 0;
	_cycles = 0;
	_oncyc = 70224;
	_dclk = 0;
	ctab[0] = 0x00FFFFFF;
	ctab[1] = 0x00AAAAAA;
	ctab[2] = 0x00555555;
	ctab[3] = 0x00000000;
	(void)type;
	//TODO: factory method to generate different ppus
}

void	ppu::cycle()
{
	if (_mmu->_oamtime)
		_mmu->dmaTransfer();
	_cycle(true);
}

void	ppu::_cycle(bool repeat)
{
	if (offcheck(2))
	{
		if (repeat == true)
			_cycle(false);
		return;
	}
//	printf("ppu %u cycles: %u\n", _mmu->PaccessAt(STAT) & 3, _cycles);
	if ((_mmu->PaccessAt(STAT) & 0x03) == 0x02)
	{
		if (spritecount < 10)
		{
			unsigned short addr = (0xFE00 + (cstate++ * 4));
			unsigned char y = _mmu->PaccessAt(addr);
			unsigned char ssize = (_mmu->PaccessAt(LCDC) & (1 << 2)) ? 16 : 8;
//			printf("ssize %u 0 < %u && %u < 160 && %u <= %u && %u <= %u\n", ssize, y, y, y, _y + 16, _y + 16, y + ssize);
			if (0 < y && y < 160 && y <= _y + 16 && _y + 16 <=y + ssize)
			{
//				printf("yes\n");
				spriteattr[spritecount][SPRITE_Y] = y;
				spriteattr[spritecount][SPRITE_X] = _mmu->PaccessAt(addr + 1);
				spriteattr[spritecount][SPRITE_T] = _mmu->PaccessAt(addr + 2);
				spriteattr[spritecount][SPRITE_A] = _mmu->PaccessAt(addr + 3);
				spritecount++;
//				printf("x %u y %u t %u\n", spriteattr[spritecount][SPRITE_X], spriteattr[spritecount][SPRITE_Y], spriteattr[spritecount][SPRITE_T]);
			}
		}
		if (cstate == 40)
		{
//			if (spritecount)
//				printf("line %u sprites %u\n", _y, spritecount);
			_mmu->STATupdate(0x03);
			printf("Mode 3\n");
			cstate = 7;
			ctile = 0;
		}
	}
	else if ((_mmu->PaccessAt(STAT) & 0x03) == 0x03)
	{
		if (_x == 160)
		{
			spritecount = 0;
			_mmu->STATupdate(0x00);
			printf("HBlank\n");
		}
		else
			drawpix();
	}
	else if (_cycles >= 456)
	{
		_cycles -= 456;
		_y = _y + 1;
		palletcalc();
		if (_y < 144 || _y == 154)
		{
			if (_y == 154)
				_y = 0;
			printf("Mode 2\n");
			cstate = 0;
			_mmu->STATupdate(0x02);
		}
		else if (_y == 144)
		{
			unsigned char IF = _mmu->PaccessAt(0xFF0F);
			IF |= 1;
			_mmu->writeTo(0xFF0F, IF);
			_mmu->STATupdate(0x01);
			printf("VBlank\n");
		}
		_x = 0;
		_mmu->writeTo(LY, _y);
	}
	_cycles += 2;
	if (repeat)
		_cycle(false);
}

void	ppu::drawpix()
{
	sx = _mmu->PaccessAt(SCX);
	sy = _mmu->PaccessAt(SCY);
	wx = _mmu->PaccessAt(WX);
	wy = _mmu->PaccessAt(WY);
	lcdc = _mmu->PaccessAt(LCDC);
	switch (cstate)
	{
		case 0:
			gettnum();
			cstate++;
			break;
		case 1:
			getbyte();
			cstate++;
			break;
		case 2:
			getbyte();
			cstate = (_dclk && !iswin) ? 0 : cstate + 1;
			break;
		case 3:
			genBuf(bwtile[ctile++]);
			spritecheck();
			break;
		case 4:
			tilenum = spriteattr[spriteindex][SPRITE_T];
			cstate++;
			break;
		case 5:
			getsprite();
			cstate++;
			break;
		case 6:
			getsprite();
			cstate = 0;
			break;
		case 7:
			_dclk = 6 + (sx % 8);
			gettnum();
			cstate = 1;
			break;
	}
	_drawpix(true);
}

void	ppu::_drawpix(bool repeat)
{
	if (_dclk)
	{
		_dclk--;
		if (repeat == true)
			_drawpix(false);
		return ;
	}
	if (_x == 160)
		return ;
	unsigned char ex = (_x + sx) % 8;
	if ((_y * 160) + _x > 23040)
		printf("bad pix at %u %u\n", _x, _y);
	unsigned char i = sprite ? _x - (spriteattr[spriteindex][SPRITE_X] - 8) : 0;
	printf("_x %u sx %u ex %u i %u\n", _x, sx, ex, i);
	if (sprite == true && (lcdc & 2) && sptile[i] && (!(spriteattr[spriteindex][SPRITE_A] & (1 << 7)) || !bwtile[_x / 8][ex] || !(lcdc & 1)))
			pixels[(_y * 160) + _x] = (spriteattr[spriteindex][SPRITE_A] & (1 << 4)) ? obp1[sptile[i]] : obp0[sptile[i]];
	else if (lcdc & 1)
		pixels[(_y * 160) + _x] = bgp[bwtile[_x / 8][ex]];
	else
		pixels[(_y * 160) + _x] = 0x00FFFFFF;
	_x++;
	if (i >= 7)
		sprite = false;
	if (repeat == true)
		_drawpix(false);
}

void	ppu::gettnum()
{
	unsigned short map;
//	unsigned char ex = _dclk ? _x : _x + 8;
	if ((lcdc & (1 << 5)) && _y >= wy && _x >= (wx - 7))
	{
		map = (lcdc & (1 << 6)) ? 0x9C00 : 0x9800;
		tilenum = _mmu->PaccessAt(map + ((((_y - wy) / 8) * 32) + (((_x - wx - 7) / 8))));
		iswin = true;
		if (wx == _x && !_dclk)
			_dclk = 6 + (wx % 8);
	}
	else
	{
		map = (lcdc & (1 << 3)) ? 0x9C00 : 0x9800;
		tilenum = _mmu->PaccessAt(map + ((((_y + sy) / 8) * 32) + (((_x + sx) / 8))));
		iswin = false;
	}
}

void	ppu::getbyte()
{
	if (iswin)
	{
		(lcdc & (1 << 4)) ? fetch8((_y - wy) % 8) : fetch9((_y - wy) % 8);
	}
	else
	{
		(lcdc & (1 << 4)) ? fetch8((_y + sy) % 8) : fetch9((_y + sy) % 8);
	}
}

void	ppu::spritecheck()
{
	if (spritecount && (lcdc & 2))
	{
		unsigned ex = _x + 8;
		for (unsigned char i = 0; i < spritecount; i++)
			if (!sprite && spriteattr[i][SPRITE_X] <= ex && ex < (spriteattr[i][SPRITE_X] + 8))
			{
//					printf("new sprite %u x %u\n", i, spriteattr[i][SPRITE_X]);
					cstate = 4;
					spriteindex = i;
					unsigned char sub = iswin ? (255 - wx) % 8 : sx % 8;
					_dclk = 11 - (sub > 5) ? 5 : sub;
					sprite = true;
					return ;
			}
	}
	cstate = 0;
	sprite = false;
}

void	ppu::getsprite()
{
	printf("sprite\n");
	unsigned ey = _y - (spriteattr[spriteindex][SPRITE_Y] - 16);
	if (lcdc & (1 << 2))
	{
		if (spriteattr[spriteindex][SPRITE_A] & (1 << 6))
			ey = 16 - ey;
		if (ey < 8)
		{
			tilenum = (spriteattr[spriteindex][SPRITE_A] & (1 << 6)) ? tilenum | 0x01 : tilenum & 0xFE;
			printf("ey %u\n", ey);
			fetch8(ey);
		}
		else
		{
			tilenum = (spriteattr[spriteindex][SPRITE_A] & (1 << 6)) ? tilenum & 0xFE : tilenum | 0x01;
			printf("ey %u\n", ey - 8);
			fetch8(ey - 8);
		}
	}
	else
	{
		printf("ey %u\n", ey);
		fetch8(ey);
	}
	if (!(cstate % 2))
	{
//		printf("sprite bufgen\n");
		genBuf(sptile);
		if (spriteattr[spriteindex][SPRITE_Y] & (1 << 5))
			for (unsigned char i = 0; i < 8; i++)
			{
				unsigned char t = sptile[i];
				sptile[i] = sptile[7 - i];
				sptile[7 - i] = t;
			}
	}
}

void	ppu::fetch8(unsigned char offset)
{
	printf("x %u y %u offset %u\n", _x, _y, offset);
	if (cstate % 2)
		tbyte[0] = _mmu->PaccessAt((0x8000 + (tilenum * 16)) + (2 * offset));
	else
		tbyte[1] = _mmu->PaccessAt(1 + ((0x8000 + (tilenum * 16)) + (2 * offset)));
}

void	ppu::fetch9(unsigned char offset)
{
	char tn = tilenum;
	if (cstate % 2)
		tbyte[0] = _mmu->PaccessAt((0x9000 + (tn * 16)) + (2 * offset));
	else
		tbyte[1] = _mmu->PaccessAt(1 + ((0x9000 + (tn * 16)) + (2 * offset)));
}

void	ppu::genBuf(unsigned char *t)
{
	printf("bufgen\n");
//	for (unsigned i = 0; i < 8; i++)
//		printf("%u", (tbyte[0] & (1 << i)) ? 1 : 0);
//	printf(" ");
//	for (unsigned i = 0; i < 8; i++)
//		printf("%u", (tbyte[1] & (1 << i)) ? 1 : 0);
//	printf("\n");
	for (unsigned char i = 0; i < 8; i++)
	{
		t[i] = ((tbyte[0] >> (7 - i) & 1)) | (((tbyte[1] >> (7 - i)) & 1) << 1);
//		printf("%u ", t[i]);
	}
//	printf("\n");
}

void	ppu::palletcalc()
{
//	printf("pallette gen\n");
	unsigned char _bgp = _mmu->PaccessAt(BGP);
	unsigned char _obp0 = _mmu->PaccessAt(OBP0);
	unsigned char _obp1 = _mmu->PaccessAt(OBP1);
	bgp[0] = ctab[_bgp & 0x03];
	bgp[1] = ctab[(_bgp >> 2) & 0x03];
	bgp[2] = ctab[(_bgp >> 4) & 0x03];
	bgp[3] = ctab[(_bgp >> 6) & 0x03];
	obp0[0] = ctab[_obp0 & 0x03]; 
	obp0[1] = ctab[(_obp0 >> 2) & 0x03];
	obp0[2] = ctab[(_obp0 >> 4) & 0x03];
	obp0[3] = ctab[(_obp0 >> 6) & 0x03];
	obp1[0] = ctab[_obp1 & 0x03];
	obp1[1] = ctab[(_obp1 >> 2) & 0x03];
	obp1[2] = ctab[(_obp1 >> 4) & 0x03];
	obp1[3] = ctab[(_obp1 >> 6) & 0x03];
//	printf("bgp: ");
//	for (unsigned char i = 0; i < 8; i++)
//		printf("%u", (_bgp & (1 << i)) ? 1 : 0);
//	printf("\n");
//	for (unsigned char i = 0; i < 4; i++)
//		printf("bgp 0x%08X\n", bgp[i]);
//	printf("obp0: ");
//	for (unsigned char i = 0; i < 8; i++)
//		printf("%u", (_obp0 & (1 << i)) ? 1 : 0);
//	printf("\n");
//	for (unsigned char i = 0; i < 4; i++)
//		printf("obp0 0x%08X\n", obp0[i]);
//	printf("obp1: ");
//	for (unsigned char i = 0; i < 8; i++)
//		printf("%u", (_obp1 & (1 << i)) ? 1 : 0);
//	printf("\n");
//	for (unsigned char i = 0; i < 4; i++)
//		printf("obp1 0x%08X\n", obp1[i]);
}
