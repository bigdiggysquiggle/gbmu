#include "print_debug.hpp"
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

#define SP_PAL (spriteattr[spriteindex][SPRITE_A] & 16)
#define SP_XFLIP (spriteattr[spriteindex][SPRITE_A] & 32)
#define SP_YFLIP (spriteattr[spriteindex][SPRITE_A] & 64)
#define SP_PRIO	(spriteattr[spriteindex][SPRITE_A] & 128)

#define BGW_ON (lcdc & 1)
#define SPRITE_ON (lcdc & 2)
#define SPRITE_S (lcdc & 4)
#define BG_MAP (lcdc & 8)
#define TDAT (lcdc & 16)
#define WIN_ON (lcdc & 32)
#define WIN_MAP (lcdc & 64)
#define LCD_ON (lcdc & 128)

//_oncyc is used as a delay because any time the LCD gets enabled
//it takes a whole frame before it starts drawing pixels.
//ctab is the colourtab used to generate the pallets.
//_dclk delays the LCD clock by however many cycles.
//_sclk is a separate delay used by the sprite handling logic
//to delay the LCD clock. I'd like to combine them again but I need
//to nail down the logic perfectly first.
ppu::ppu(std::shared_ptr<mmu> mem, unsigned char type) : _mmu(mem)
{
	for (unsigned i; i < 23040; i++)
		pixels[i] = 0x00FFFFFF;
	_x = 0;
	_y = 0;
	spritecount = 0;
	_cycles = 0;
	_oncyc = 70224;
	_dclk = 6;
	_sclk = 0;
	ctab[0] = 0x00FFFFFF;
	ctab[1] = 0x00AAAAAA;
	ctab[2] = 0x00555555;
	ctab[3] = 0x00000000;
	bwtile.setSize(8);
	sptile.setSize(8);
	(void)type;
	//TODO: factory method to generate different ppus
}

//for each 4 cycles of the CPU, the PPU does 2 things. Hence
//_cycle accepting a boolean argument. It makes it easier that way

void	ppu::cycle()
{
	if (_mmu->_oamtime)
		_mmu->dmaTransfer();
	_cycle(true);
}

//offcheck is define in the header file. It makes sure the LCD is enabled
//and handles the 1 frame delay before drawing happens.

void	ppu::_cycle(bool repeat)
{
	if (offcheck(2))
	{
		if (repeat == true)
			_cycle(false);
		return;
	}
//	PRINT_DEBUG("ppu %u cycles: %u", _mmu->PaccessAt(STAT) & 3, _cycles);

//	In mode 2 the PPU scans OAM to find sprites that appear on the current
//	scanline. It can only display up to 10 sprites on any given line so checks
//	are in place. Sprites can be 8x8 or 8x16 pixels as determined by a bit
//	in the LCDC register. cstate is used here to keep track of where we are in OAM memory.
//	It gets multiplied by 4 cuz each sprite has 4 bytes associated with it.
//	SPRITE_Y is the scanline its first line appears on + 16. SPRITE_X is the place in
//	the current scanline it starts being drawn + 8. SPRITE_T is the tile number that the
//	sprite uses. SPRITE_A is a set of flags that determine the attributes of the sprite.
	if ((_mmu->PaccessAt(STAT) & 0x03) == 0x02)
	{
		if (spritecount < 10)
		{
			unsigned short addr = (0xFE00 + (cstate++ * 4));
			unsigned char y = _mmu->PaccessAt(addr);
			unsigned char ssize = (_mmu->PaccessAt(LCDC) & (1 << 2)) ? 16 : 8;
//			PRINT_DEBUG("ssize %u 0 < %u && %u < 160 && %u <= %u && %u <= %u", ssize, y, y, y, _y + 16, _y + 16, y + ssize);
			if (0 < y && y < 160 && y <= _y + 16 && _y + 16 <=y + ssize)
			{
//				PRINT_DEBUG("yes");
				spriteattr[spritecount][SPRITE_Y] = y;
				spriteattr[spritecount][SPRITE_X] = _mmu->PaccessAt(addr + 1);
				spriteattr[spritecount][SPRITE_T] = _mmu->PaccessAt(addr + 2);
				spriteattr[spritecount][SPRITE_A] = _mmu->PaccessAt(addr + 3);
				spritecount++;
//				PRINT_DEBUG("x %u y %u t %u", spriteattr[spritecount][SPRITE_X], spriteattr[spritecount][SPRITE_Y], spriteattr[spritecount][SPRITE_T]);
			}
		}
		if (cstate == 40)
		{
//			if (spritecount)
//				PRINT_DEBUG("line %u sprites %u", _y, spritecount);
			_mmu->STATupdate(0x03);
//			PRINT_DEBUG("Mode 3");
			cstate = 7;
			ctile = 0;
		}
	}
	//each scanline is 160 pixels long. If I'm at 160 I know I need to HBLANK
	else if ((_mmu->PaccessAt(STAT) & 0x03) == 0x03)
	{
		if (_x == 160)
		{
			spritecount = 0;
			_dclk = 6;
			_mmu->STATupdate(0x00);
			bwtile.flush();
//			PRINT_DEBUG("HBlank");
		}
		else
			drawpix();
	}
	//allows for the scanline number to increment during VBLANK. Even though the resolution is
	//160x144, the processor still tracks another 10 extra blank lines as VBLANK occurs.
	else if (_cycles >= 456)
	{
		_cycles -= 456;
		_y = _y + 1;
		palletcalc();
		if (_y < 144 || _y == 154)
		{
			if (_y == 154)
				_y = 0;
//			PRINT_DEBUG("Mode 2");
			cstate = 0;
			_mmu->STATupdate(0x02);
		}
		_x = 0;
		_mmu->writeTo(LY, _y);
	}
	_cycles += 2;
	if (repeat)
		_cycle(false);
}

// Uses cstate to keep track of which type of memory access
// is occurring. The first time through its access cycle on
// any given scanline it reads the number of the current
// tile, then it gets the first byte that generates pixels
// for the current line, then the second byte. It then
// completely disregards this and, starting from the first
// tile, goes through the cycle described above, then has
// a 2 cycle window where sprite information can be accessed
// (which has its own associated delays which would get
// inserted at this point in the cycle).
// For more detailed info, consult the article I have linked
// in the README about the nitty gritty details as well as
// the reddit post linked in the README that details how
// sprite timing works.

void	ppu::drawpix()
{
	sx = _mmu->PaccessAt(SCX);
	sy = _mmu->PaccessAt(SCY);
	wx = _mmu->PaccessAt(WX);
	wy = _mmu->PaccessAt(WY);
	lcdc = _mmu->PaccessAt(LCDC);
	_drawpix(true);
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
			PRINT_DEBUG("ctile %u", ctile);
			ctile++;
			if (!_x && !iswin)
			{
				try{genBuf(bwtile);}
				catch (const char *e)
				{printf("BG %s\n", e);}
				cstate = 0;
			}
			else
				cstate++;
			break;
		case 3:
			try{genBuf(bwtile);}
			catch (const char *e)
			{printf("BG %s\n", e);}
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
			_dclk = _dclk + (sx % 8);
			gettnum();
			cstate = 1;
			break;
	}
}

void	ppu::_drawpix(bool repeat)
{
	if (_sclk)
	{
		_sclk--;
		if (repeat)
			_drawpix(false);
		return ;
	}
	unsigned char pix;
	unsigned char spix = 0;
	try {pix = bwtile.getPix();}
	catch (const char *e)
	{printf("BG %s\n", e);}
	if (_dclk)
	{
		_dclk--;
		if (repeat == true)
			_drawpix(false);
		return ;
	}
	if (_x == 160)
		return ;
//	unsigned char ex = (_x + sx) % 8;
	if ((_y * 160) + _x > 23040)
		PRINT_DEBUG("bad pix at %u %u\n", _x, _y);
//	unsigned char i = sprite ? _x - (spriteattr[spriteindex][SPRITE_X] - 8) : 0;
	if (sprite == true)
	{
		PRINT_DEBUG("_x %u sx %u\n", _x, sx);
//		PRINT_DEBUG("lcdc & 2 %u sptile[%u] %u (!spriteattr & 1<<7 %u || !bwtile %u || !lcdc & 1 %u) %u\n", SPRITE_ON, i, sptile[i], SP_PRIO, bwtile[_x / 8][ex], BGW_ON, (!SP_PRIO || !bwtile[_x / 8][ex] || !BGW_ON));
		try {spix = sptile.getPix();}
		catch (const char *e)
		{PRINT_DEBUG("Sprite %s\n", e);}
		PRINT_DEBUG("lcdc & 2 %u spix %u (!spriteattr & 1<<7 %u || !bwtile %u || !lcdc & 1 %u) %u\n", SPRITE_ON, spix, SP_PRIO, pix, BGW_ON, (!SP_PRIO || !pix || !BGW_ON));
	}
//	if (sprite == true && SPRITE_ON && sptile[i] && (!SP_PRIO || !bwtile[_x / 8][ex] || !BGW_ON))
	if (sprite == true && SPRITE_ON && spix && (!SP_PRIO || !pix || !BGW_ON))
	{
		PRINT_DEBUG("_x %u sx %u\n", _x, sx);
//		pixels[(_y * 160) + _x] = SP_PAL ? obp1[sptile[i]] : obp0[sptile[i]];
		pixels[(_y * 160) + _x] = SP_PAL ? obp1[spix] : obp0[spix];
	}
	else if (BGW_ON)
//		pixels[(_y * 160) + _x] = bgp[bwtile[_x / 8][ex]];
		pixels[(_y * 160) + _x] = bgp[pix];
	else
		pixels[(_y * 160) + _x] = 0x00FFFFFF;
	_x++;
	if (sptile[0] == 0xFF || _x == 160)
	{
		sprite = false;
		sptile.flush();
	}
	if (repeat == true)
		_drawpix(false);
}

void	ppu::gettnum()
{
	unsigned short map;
	unsigned char ex = _dclk ? _x : _x + 8;
	if (WIN_ON && _y >= wy && ex >= (wx - 7))
	{
		map = WIN_MAP ? 0x9C00 : 0x9800;
		tilenum = _mmu->PaccessAt(map + ((((_y - wy) / 8) * 32) + (((ex - wx - 7) / 8))));
		iswin = true;
		if (wx == ex && !_dclk)
			_dclk = 6 + (wx % 8);
	}
	else
	{
		map = BG_MAP ? 0x9C00 : 0x9800;
		tilenum = _mmu->PaccessAt(map + ((((_y + sy) / 8) * 32) + (((ex + sx) / 8))));
		iswin = false;
	}
}

void	ppu::getbyte()
{
	if (iswin)
	{
		TDAT ? fetch8((_y - wy) % 8) : fetch9((_y - wy) % 8);
	}
	else
	{
		TDAT ? fetch8((_y + sy) % 8) : fetch9((_y + sy) % 8);
	}
}

void	ppu::spritecheck()
{
	if (!sprite && spritecount && SPRITE_ON)
	{
		unsigned ex = _x + 8;
		for (unsigned char i = 0; i < spritecount; i++)
			if (!sprite && spriteattr[i][SPRITE_X] <= ex && ex < (spriteattr[i][SPRITE_X] + 8))
			{
				cstate = 4;
				spriteindex = i;
				_sclk = 9; //the first two cycles are happening here
				unsigned char sub = iswin ? (255 - wx) % 8 : sx % 8;
				_sclk -= (sub > 5) ? 5 : sub;
				sprite = true;
				return ;
			}
	}
	cstate = 0;
	sprite = false;
}

void	ppu::getsprite()
{
	PRINT_DEBUG("sprite");
	unsigned ey = _y - (spriteattr[spriteindex][SPRITE_Y] - 16);
	if (SPRITE_S)
	{
		if (SP_YFLIP)
			ey = 16 - ey;
		if (ey < 8)
		{
			tilenum = SP_YFLIP ? tilenum | 0x01 : tilenum & 0xFE;
			PRINT_DEBUG("16ey %u", ey);
			fetch8(ey);
		}
		else
		{
			tilenum = SP_YFLIP ? tilenum & 0xFE : tilenum | 0x01;
			PRINT_DEBUG("16ey %u", ey - 8);
			fetch8(ey - 8);
		}
	}
	else
	{
		if (SP_YFLIP)
			ey = 7 - ey;
		PRINT_DEBUG("8ey %u", ey);
		fetch8(ey);
	}
	if (!(cstate % 2))
	{
		PRINT_DEBUG("sprite bufgen\n");
		try {genBuf(sptile);}
		catch (const char *e)
		{PRINT_DEBUG("Sprite %s\n", e);}
		if (SP_XFLIP)
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
//	PRINT_DEBUG("x %u y %u offset %u", _x, _y, offset);
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

//void	ppu::genBuf(unsigned char *t)
void	ppu::genBuf(laz_e t)
{
//#ifdef DEBUG_PRINT_ON
//	PRINT_DEBUG("bufgen");
//	for (unsigned i = 0; i < 8; i++)
//		printf("%u", (tbyte[0] & (1 << i)) ? 1 : 0);
//	printf(" ");
//	for (unsigned i = 0; i < 8; i++)
//		printf("%u", (tbyte[1] & (1 << i)) ? 1 : 0);
//	printf("\n");
//#endif
	if (t[0] != 0xFF)
		throw "Error: buffer not empty";
	for (unsigned char i = 0; i < 8; i++)
	{
		t[i] = ((tbyte[0] >> (7 - i) & 1)) | (((tbyte[1] >> (7 - i)) & 1) << 1);
//		PRINT_DEBUG("%u ", t[i]);
	}
//#ifdef DEBUG_PRINT_ON
//	printf("\n");
//#endif
}

void	ppu::palletcalc()
{
//	PRINT_DEBUG("pallette gen");
	unsigned char _bgp = _mmu->PaccessAt(BGP);
	unsigned char _obp0 = _mmu->PaccessAt(OBP0);
	unsigned char _obp1 = _mmu->PaccessAt(OBP1);
//	PRINT_DEBUG("ctab:");
//	for (int i = 0; i < 4; i++)
//		PRINT_DEBUG("\t0x%08X", ctab[i]);
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
//	PRINT_DEBUG("bgp: ");
//	for (unsigned char i = 0; i < 8; i++)
//		printf("%u", (_bgp & (1 << i)) ? 1 : 0);
//	printf("\n");
//	for (unsigned char i = 0; i < 4; i++)
//		printf("\t0x%08X\n", bgp[i]);
//	PRINT_DEBUG("obp0: ");
//	for (unsigned char i = 0; i < 8; i++)
//		printf("%u", (_obp0 & (1 << i)) ? 1 : 0);
//	printf("\n");
//	for (unsigned char i = 0; i < 4; i++)
//		printf("\t0x%08X\n", obp0[i]);
//	PRINT_DEBUG("obp1: ");
//	for (unsigned char i = 0; i < 8; i++)
//		printf("%u", (_obp1 & (1 << i)) ? 1 : 0);
//	printf("\n");
//	for (unsigned char i = 0; i < 4; i++)
//		printf("\t0x%08X\n", obp1[i]);
}
