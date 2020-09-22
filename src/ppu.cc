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

ppu::ppu(std::shared_ptr<mmu> mem, unsigned char type) : _mmu(mem)
{
	for (unsigned i; i < 23040; i++)
		pixels[i] = 0x00FFFFFF;
	_x = 0;
	_y = 0;
	_cycles = 0;
	_delaycyc = 0;
	spritecount = 0;
	sprite = false;
	(void)type;
	//TODO: factory method to generate different ppus
}

void	ppu::cycle()
{
	_cycle(true);
}

void	ppu::_cycle(bool repeat)
{
	if ((_mmu->PaccessAt(STAT) & 0x03) == 0x02)
	{
		if (spritecount < 10)
		{
			unsigned short addr = (0xFE00 + (_cycles / 2));
			unsigned char y = _mmu->PaccessAt(addr);
			unsigned char ssize = (_mmu->PaccessAt(LCDC) & (1 << 2)) ? 16 : 8;
			if (0 < y && y <= 160 && (y - 16) <= _y && _y <=  (y + ssize))
			{
				spriteattr[spritecount][0] = (y - 16);
				spriteattr[spritecount][1] = _mmu->PaccessAt(addr + 1) - 8;
				spriteattr[spritecount][2] = _mmu->PaccessAt(addr + 2);
				spriteattr[spritecount][3] = _mmu->PaccessAt(addr + 3);
				spritecount++;
			}
		}
		if (_cycles >= MODE_2)
			_mmu->STATupdate(0x03);
	}
	else if ((_mmu->PaccessAt(STAT) & 0x03) == 0x03)
	{
		if (_x == 160)
		{
			_y++;
			_mmu->writeTo(LY, _y);
			_mmu->STATupdate((_y < 144) ? 0x00 : 0x01);
		}
		else
			drawpix();
	}
	else if (_cycles >= 456)
	{
		_cycles -= 456;
		_delaycyc = 0;
		if (_y < 144)
			_mmu->STATupdate(0x02);
		else
		{
			if (_y == 153)
			{
				_y = 0;
				_mmu->STATupdate(0x02);
			}
			else
				_y++;
			_mmu->writeTo(LY, _y);
		}
	}
	_cycles += 2;
	if (repeat)
		_cycle(false);
}

void	ppu::drawpix()
{
	if (_spritecyc)
	{
		_spritecyc -= 2;
		_cycles -=2;
		return ;
	}
	sx = _mmu->PaccessAt(SCX);
	sy = _mmu->PaccessAt(SCY);
	wx = _mmu->PaccessAt(WX);
	wy = _mmu->PaccessAt(WY);
	lcdc = _mmu->PaccessAt(LCDC);
	unsigned currcyc = _cycles - _delaycyc - MODE_2;
	if ((currcyc < 6 || (currcyc - 6) % 8 != 6))
		if (!(currcyc % 8))
			gettile();
	else
	{
		if (lcdc & 2)
			spritecheck();
		palletcalc();
		unsigned char i = ((_x + sx) % 8) - 1;
		while (++i < 8)
		{
			unsigned char j = 0;
			if (sprite)
				j = _x - spriteattr[spriteindex][1];
			if (sprite && (lcdc & 2) && stile[j] && (!(spriteattr[spriteindex][3] & (1 << 7)) || !tile[i] || !(lcdc & 1)))
				pixels[(_y * 160) + _x] = (spriteattr[spriteindex][3] & (1 << 4)) ? obp1[stile[j]] : obp0[stile[j]];
			else if (lcdc & 1)
				pixels[(_y * 160) + _x] = bgp[tile[i]];
			else
				pixels[(_y * 160) + _x] = 0x00FFFFFF;
			_x++;
		}
	}
}


void	ppu::gettile()
{
	unsigned short map;
	if (_y >= wy && _x >= (wx - 7) && (lcdc & (1 << 5)))
	{
		map = (lcdc & (1 << 6)) ? 0x9C00 : 0x9800;
		tilenum = _mmu->PaccessAt(map + (((_y - wy) * 160) + ((_x - wx - 7))));
		(lcdc & (1 << 4)) ? fetch8((_y - wy) % 8, tile) : fetch9((_y - wy) % 8, tile);
	}
	else
	{
		map = (lcdc & (1 << 3)) ? 0x9C00 : 0x9800;
		tilenum = _mmu->PaccessAt(map + ((_y + sy) * 160) + (_x + wx - 7));
		(lcdc & (1 << 4)) ? fetch8((_y + sy) % 8, tile) : fetch9((_y + sy) % 8, tile);
	}
}

void	ppu::spritecheck()
{
	for (unsigned char i = 0; i < spritecount; i++)
		if (spriteattr[i][1] <= _x && _x <= (spriteattr[i][1] + 8))
			{
				getsprite(i);
				return ;
			}
	sprite = false;
}

void	ppu::getsprite(unsigned char i)
{
	tilenum = spriteattr[i][2];
	if (lcdc & (1 << 2))
	{
		unsigned ey = _y - spriteattr[i][1];
		if (spriteattr[i][3] & (1 << 6))
			ey = 16 - ey;
		if (ey < 8)
		{
			tilenum = (spriteattr[i][3] & (1 << 6)) ? tilenum | 0x01 : tilenum & 0xFE;
			fetch8(ey, stile);
		}
		else
		{
			tilenum = (spriteattr[i][3] & (1 << 6)) ? tilenum & 0xFE : tilenum | 0x01;
			fetch8(ey - 8, stile);
		}
	}
	else
		fetch8(_y - spriteattr[i][1], stile);
	if (spriteattr[i][3] & (1 << 5))
		for (unsigned char i = 0; i < 8; i++)
		{
			unsigned char t = stile[i];
			stile[i] = stile[7 - i];
			stile[7 - i] = t;
		}
	sprite = true;
	spriteindex = i;
	unsigned delay;
	if (_y >= wy && _x >= (wx - 7))
		delay = (spriteattr[i][1] + (255 - wx)) % 8;
	else
		delay = (spriteattr[i][1] + sx) % 8;
	delay = 11 - (delay < 5) ? delay : 5;
	_spritecyc = delay;
	_delaycyc += delay;
}

void	ppu::fetch8(unsigned char offset, unsigned char *t)
{
	tbyte[0] = _mmu->PaccessAt((0x8000 + tilenum) + (2 * offset));
	tbyte[1] = _mmu->PaccessAt(1 + ((0x8000 + tilenum) + (2 * offset)));
	for (unsigned char i = 0; i < 8; i++)
		t[i] = ((tbyte[0] >> (7 - i) & 1)) | (((tbyte[1] >> (7 - i)) & 1) << 1);
}

void	ppu::fetch9(unsigned char offset, unsigned char *t)
{
	tbyte[0] = _mmu->PaccessAt((0x9000 + tilenum) + (2 * offset));
	tbyte[1] = _mmu->PaccessAt(1 + ((0x9000 + tilenum) + (2 * offset)));
	for (unsigned char i = 0; i < 8; i++)
		t[i] = ((tbyte[0] >> (7 - i) & 1)) | (((tbyte[1] >> (7 - i)) & 1) << 1);
}

void	ppu::palletcalc()
{
	unsigned char _bgp = _mmu->PaccessAt(BGP);
	unsigned char _obp0 = _mmu->PaccessAt(OBP0);
	unsigned char _obp1 = _mmu->PaccessAt(OBP1);
	bgp[0] = _bgp & 0x02;
	bgp[1] = (_bgp >> 2) & 0x02;
	bgp[2] = (_bgp >> 4) & 0x02;
	bgp[3] =  (_bgp >> 6) & 0x02;
	obp0[0] = _obp0 & 0x02; 
	obp0[1] = (_obp0 >> 2) & 0x02;
	obp0[2] = (_obp0 >> 4) & 0x02;
	obp0[3] = (_obp0 >> 6) & 0x02;
	obp1[0] = _obp1 & 0x02;
	obp1[1] = (_obp1 >> 2) & 0x02;
	obp1[2] = (_obp1 >> 4) & 0x02;
	obp1[3] = (_obp1 >> 6) & 0x02;
}
