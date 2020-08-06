#include "ppu.hpp"
#include <unistd.h>

//vertical refresh ever 70224 clocks (140448 in GBC
//double speed mode): 59,7275 Hz
//
//A scanline normally takes 456 clocks (912 clocks in double
//speed mode) to complete. A scanline
//starts in mode 2, then goes to mode 3 and, when the LCD
//controller has finished drawing the line
//(the timings depend on lots of things) it goes to mode 0.
//During lines 144-153 the LCD controller is
//in mode 1. Line 153 takes only a few clocks to complete
//(the exact timings are below). The rest ofthe clocks of
//line 153 are spent in line 0 in mode 1!

ppu::ppu(std::shared_ptr<mmu> unit) : _mmu(unit)
{
	_y = 255;
	_mmu->writeTo(0xFF44, 0);
	_sx = _mmu->PaccessAt(0xFF43);
	_sy = _mmu->PaccessAt(0xFF42);
	_wx = _mmu->PaccessAt(0xFF4B);
	_wy = _mmu->PaccessAt(0xFF4A);
	LCDC = _mmu->PaccessAt(0xFF40);
	STAT = _mmu->PaccessAt(0xFF41);
	_cycles = 70224;
	_off = 1;
	_modenxt = 2;
	unsigned i = 0;
	while (i < 23040)
		pixels[i++] = 0xFFFFFF;
	i = 3;
//	while (i < 92160)
//	{
//		if (pixels[i] != 255)
//		{
//			printf("bad pixel opacity\n");
//			exit(1);
//		}
//		i+=4;
//	}
}

//HBL mode 0
//VBL mode 1 (LY=144)
//mode 2 OAM access
//mode 3 data transfer

//0 white 1 light gray 2 dark grey 3 black

unsigned ctab[] = {
	0x00FFFFFF, 0x00AAAAAA, 0x00555555, 0x00000000};
//The window is an alternate background that can be displayed
//over the normal background. This
//window is drawn by resetting the LCD background state
//machine and changing to the window map.
//This results in only being able to show the upper left
//part of the window map (and no wrapping,
//logically). Priorities between window and sprites are
//shared with normal background.
//The behaviour of this registers is a bit strange
//8.16.1. Window Display Depending on WX and WY
//The window is visible (if enabled) when WX=7-166, WY=0-143.
//A position of WX=7, WY=0
//locates the window at upper left corner of the screen,
//covering normal background. Other values for
//WX will give strange results:

//Each sprite usually pauses for 11 - min(5, (x + SCX) mod 8)
//dots. Because sprite fetch waits for background fetch to
//finish, a sprite's cost depends on its position relative to
//the left side of the background tile under it. It's greater
//if a sprite is directly aligned over the background tile,
//less if the sprite is to the right. If the sprite's left
//side is over the window, use 255 - WX for SCX in this
//formula

void	ppu::VBlank()
{
	_cycles += 456;
	unsigned char IF = _mmu->accessAt(0xFF0F);
	_mmu->writeTo(0xFF0F, IF | 0x01);
}

void	ppu::printSprite(unsigned char sprite[16], unsigned char attr[4], unsigned char lcdc)
{
	if (attr[1] >= 160)
		return ;
	unsigned char obp = attr[3] & (1 << 4) ? _mmu->PaccessAt(0xFF49) : _mmu->PaccessAt(0xFF48);
	unsigned obpt[] = {
		0x000000, ctab[(obp >> 2) & 3],
		ctab[(obp >> 4) & 3], ctab[(obp >> 6)]};
	unsigned char lower;
	unsigned char upper;
	unsigned char bgp = _mmu->PaccessAt(0xFF47);
	if (!(attr[3] & (1 << 6)))
	{
		lower = sprite[2 * (_y - attr[0])];
		upper = sprite[(2 * (_y - attr[0])) + 1];
	}
	else
	{
		lower = sprite[15 - (2 *(_y - attr[0]))];
		upper = sprite[14 - (2 * (_y - attr[0]))];
	}
	unsigned char i = -1;
	unsigned char c;
	if (!(attr[3] & (1 << 5)))
		while (++i < 8)
		{
			c = ((lower >> (7 - i)) & 1) | (((upper >> (7 - i)) & 1) << 1);
			if (!c || attr[i] + i > 160)
				continue ;
			if (!(attr[3] & (1 << 7)) || pixels[(_y * 160) + attr[1] + i] == ctab[bgp & 0x03])
				pixels[(_y * 160) + attr[1] + i] = obpt[c];
		}
	else
		while (++i < 8)
		{
			c = ((lower >> i) & 1) | (((upper >> i) & 1) << 1);
			if (!c || attr[i] + i > 160)
				continue ;
			if (!(attr[3] & (1 << 7)) || pixels[(_y * 160) + attr[1] + i] == ctab[bgp & 0x03])
				pixels[(_y * 160) + attr[1] + i] = obpt[c];
		}
}

void	ppu::printSprites16(unsigned char lcdc)
{
	unsigned char i = spritecount;
	unsigned short addr = 0x8000;
	unsigned char sprite[16];
	unsigned char byte;
	while (i-- > 0)
	{
		byte = 0;
		while (byte < 16)
		{
			if ((_y - spriteattr[i][0]) < 8)
				sprite[byte] = _mmu->PaccessAt(addr + byte + ((spriteattr[i][2] & 0xFE) * 16));
			else
				sprite[byte] = _mmu->PaccessAt(addr + byte + ((spriteattr[i][2] | 0x01) * 16));
			byte++;
		}
		printSprite(sprite, spriteattr[i], lcdc);
	}
}

void	ppu::printSprites8(unsigned char lcdc)
{
	unsigned char i = spritecount;
	unsigned short addr = 0x8000;
	unsigned char sprite[16];
	unsigned char byte;
	while (i-- > 0)
	{
		byte = 0;
		while (byte < 16)
		{
			sprite[byte] = _mmu->PaccessAt(addr + byte + (spriteattr[i][2] * 16));
			byte++;
		}
		printSprite(sprite, spriteattr[i], lcdc);
	}
}

void	ppu::readOAM(unsigned char lcdc)
{
	unsigned short	addr = 0xFE00;
	spritecount = 0;
	unsigned char	ssize = (lcdc >> 2) & 1 ? 16 : 8;
	while (addr < 0xFE9F && spritecount < 10)
	{
		spriteattr[spritecount][0] = _mmu->PaccessAt(addr);
		if (0 < spriteattr[spritecount][0] && spriteattr[spritecount][0] <= 160)
		{
			spriteattr[spritecount][0] -= 16;
			//because of the way I handled hblanking I need to
			//compare against _y + 1 for this function so it
			//will be on the correct scanline when it prints
			//the sprite
			if (spriteattr[spritecount][0] <= (_y + 1) && (_y + 1) < spriteattr[spritecount][0] + ssize)
			{
				spriteattr[spritecount][1] = _mmu->PaccessAt(addr + 1) - 8;
				spriteattr[spritecount][2] = _mmu->PaccessAt(addr + 2);
				spriteattr[spritecount][3] = _mmu->PaccessAt(addr + 3);
				spritecount++;
			}
		}
		addr += 4;
	}
}

void	ppu::readSprites(unsigned char lcdc)
{
	_pause = 0;
	if (lcdc & (1 << 6))
	{
		for (unsigned char d = 0; d < spritecount; d++)
			if ((spriteattr[d][1] + 8) > (_wx - 7) && (spriteattr[d][0] + 8) > _wy)
				_pause += (11 - (5 > ((spriteattr[d][1] + (255 - _wx))% 8) ? ((spriteattr[d][1] + (255 - _wx)) % 8) : 5));
	}
	else
		for (unsigned char d = 0; d < spritecount; d++)
			_pause += (11 - (5 > ((spriteattr[d][1] + _sx)% 8) ? ((spriteattr[d][1] + _sx) % 8) : 5));
	lcdc & (1 << 2) ? printSprites16(lcdc) : printSprites8(lcdc);
}
//turning on window adds delay of 6
//y / 8 for current line's tiles
//consider placing this in mmu
//possible optimization, vram writes could calculate the new
//byte val as they get written
void	ppu::refreshTiles()
{
	unsigned short mapaddr;
	unsigned tile = 0;
	unsigned char line;
	unsigned char pix;
	mapaddr = 0x8000;
	while (mapaddr < 0x97FF)//since mapaddr increments several times between each check this condition is fine
	{
		for (line = 0; line < 8; line++)//tiles are 2 bytes per line
		{
			unsigned char lower = _mmu->accessAt(mapaddr++);
			unsigned char upper = _mmu->accessAt(mapaddr++);
			unsigned char i = 8;
			pix = 0;
			while (i--)
			{
				tiles[tile][line][pix++] = (((lower >> i) & 1) + (((upper >> i) & 1) << 1));
			}
		}
		tile++;
	}
	_mmu->vramWrite = false;
}

void	ppu::getTiles9(unsigned short addr, unsigned char x, unsigned char y, unsigned char dis)
{
	char	tab[32];
	for (int i = 0; i < 32; i++)
		tab[i] = _mmu->accessAt(addr++);
	unsigned char bgp = _mmu->PaccessAt(0xFF47);
	unsigned bgc[] = {
		ctab[bgp & 3], ctab[(bgp >> 2) & 3],
		ctab[(bgp >> 4) & 3], ctab[(bgp >> 6)]};
	//theoretically my xscroll plus the current pixel over 8
	//should be which tile in the tab I'm looking at.
	//Because it's a signed value using 0x9000 addressing,
	//I'm looking at tile banks 1 and 2 (not bank 0), thus
	//the value tab returns plus 256 should give me the actual
	//tile I'm after. _y mod 8 should give me the line number
	//I'm looking for.
	unsigned char pix = 0;
	while (pix + dis < 160)
	{
		for (unsigned char tpix = 0; tpix < 8; tpix++)
			pixels[pix + dis + tpix + (_y * 160)] = bgc[tiles[256 + tab[(pix + tpix + x)/8]][y % 8][tpix]];
		pix += 8;
	}
}

//factor in window drawing
//do a background draw if background is enabled, then
//window draw if window is enabled
//change this to use unsigned ints as the pallet data
void	ppu::getTiles8(unsigned short addr, unsigned char x, unsigned char y, unsigned char dis)
{
	unsigned char tab[32];
	for (int i = 0; i < 32; i++)
		tab[i] = _mmu->accessAt(addr++);
	addr -= 32;
	unsigned char bgp = _mmu->PaccessAt(0xFF47);
	unsigned bgc[] = {
		ctab[bgp & 3], ctab[(bgp >> 2) & 3],
		ctab[(bgp >> 4) & 3], ctab[(bgp >> 6)]};
	//theoretically my xscroll plus the current pixel over 32
	//should be which tile in the tab I'm looking at. that
	//tile is the index for my overall tiles table. _y mod 8
	//should give me the line number I'm looking for.
	unsigned char pix = 0;
	while (pix + dis < 160)
	{
		for (unsigned char tpix = 0; tpix < 8; tpix++)
		{
			pixels[pix + dis + tpix + (_y * 160)] = bgc[tiles[tab[(pix + tpix + x)/8]][y % 8][tpix]];
		}
		pix += 8;
	}
}
/*
void	ppu::readTiles(unsigned char lcdc)
{
	unsigned short mapaddr;
	if (_mmu->vramWrite == true)
		refreshTiles();
	if (lcdc & (1 << 5))
		mapaddr = STAT & (1 << 6) ? 0x9C00 : 0x9800;
	else
		mapaddr = STAT & (1 << 3) ? 0x9C00 : 0x9800;
	unsigned char y;
	unsigned char x;
	unsigned char dis = 0;
	if (lcdc & (1 << 6))
	{
		y = _wy;
		x = _wx;
		dis = _wx;
	}
	else
	{
		y = _sy;
		x = _sx;
	}
	y += _y;
	mapaddr += (unsigned short)((y >> 3) * 32);
	lcdc & (1 << 4) ? getTiles8(mapaddr, x, y, dis) : getTiles9(mapaddr, x, y, dis);
}*/

void	ppu::readTiles(unsigned char lcdc)
{
	unsigned short mapaddr;
	if (_mmu->vramWrite == true)
		refreshTiles();
//	if (lcdc & (1 << 5))
//		mapaddr = STAT & (1 << 6) ? 0x9C00 : 0x9800;
//	else
	mapaddr = (STAT & (1 << 3)) ? 0x9C00 : 0x9800;
	unsigned char y;
	unsigned char x;
	unsigned char dis = 0;
//	if (lcdc & (1 << 6))
//	{
//		y = _wy;
//		x = _wx;
//		dis = _wx;
//	}
//	else
//	{
		y = _sy;
		x = _sx;
//	}
	y += _y;
	mapaddr += (unsigned short)((y >> 3) * 32);
	(lcdc & (1 << 4)) ? getTiles8(mapaddr, x, y, dis) : getTiles9(mapaddr, x, y, dis);
	if (!(lcdc & 1 << 5))
		return ;
	mapaddr = (STAT & (1 << 6)) ? 0x9C00 : 0x9800;
	y = _wy;
	x = _wx;
	dis = _wx;
	y += _y;
	mapaddr += (unsigned short)((y >> 3) * 32);
	(lcdc & (1 << 4)) ? getTiles8(mapaddr, x, y, dis) : getTiles9(mapaddr, x, y, dis);
}

void	ppu::renderLine(unsigned char lcdc)
{
	_sx = _mmu->PaccessAt(0xFF43);
	_sy = _mmu->PaccessAt(0xFF42);
	_wx = _mmu->PaccessAt(0xFF4B);
	_wy = _mmu->PaccessAt(0xFF4A);
//	if (lcdc & (1 << 5)) //since this pauses the actual sys clock I might be able to get away with just not doing it
//		_pause = 6;
	if (lcdc & 1) //works different for cgb
		readTiles(lcdc);
	if (lcdc & 2 && spritecount)
		readSprites(lcdc);
}

// 173.5 + (xscroll % 8)
//entire frame is 70224 cycles
//each line is 456 cycles
//cycles through modes 2, 3, 0 once per line
//0 is off screen for sprites
//either have hblank print all at once or
//have pixel draw function that runs each cycle
//skip the first frame
int		ppu::frameRender(unsigned char cyc)
{
	unsigned char	lcdc = _mmu->PaccessAt(0xFF40);
	unsigned char	mode = _mmu->PaccessAt(0xFF41);
	_cycles = cyc - _cycles;
  	unsigned char y = _mmu->PaccessAt(0xFF44);
//	unsigned char currmode = mode & 0x03;
	mode &= ~(0x03);
	mode |= _modenxt;
	unsigned i = 0;
//	switch(currmode)
	switch(_modenxt)
	{
		case 0://move to mode 1/2
			if (_y < 144)
				y++;
	  		if (y == 144)
			{
				_y = 144;
				VBlank();
				_modenxt = 1;
				_mmu->writeTo(0xFF44, y);
				return 1;
			}
			_modenxt = 2;
			_mmu->writeTo(0xFF44, y);
			readOAM(lcdc);
			_cycles += 80;
			break;
		case 1://move to mode 2
			if (_y <= 153)
			{
				_cycles = cyc - _cycles;
				_cycles += 456;
				_y++;
				_modenxt = 1;
				_mmu->writeTo(0xFF44, (_y == 154) ? 0 : _y);
			}
			else
			{
				_modenxt = 2;
				readOAM(lcdc);
				_cycles += 80;
			}
			break;
		case 2://move to mode 3
			_modenxt = 3;
			if (y != _y || (lcdc & 1 << 5 && !(LCDC & 1 << 5)))
			{
				_y = y;
				renderLine(lcdc);
			}
			_hblank = 208 - _pause;
			_cycles += 168; //168 - 291 depending on sprite count
			break;
		case 3://moving to mode 0
//			printf("mode 0 hblank\n");
			_cycles += _hblank; //85 - 208 depending on mode 3 time
			_modenxt = 0;
			break;
//		}
//		auto stop = std::chrono::high_resolution_clock::now();
//		auto dur = std::chrono::duration_cast<std::chrono::microseconds>(stop-start);
//		std::cout << "graphics switch: " << dur.count() << std::endl;
	}
	_mmu->STATupdate(mode);
	LCDC = lcdc;
	STAT = mode;
//	printf("\n");
		//check lcdc/stat changes here
	return 0;
}
