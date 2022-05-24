#include "print_debug.hpp"
#include "mmu.hpp"

//replace this with a 100 byte array that
//the program fills with the contents of a file

unsigned char _booter[] = {
	0x31, 0xfe, 0xff, 0xaf, 0x21, 0xff, 0x9f, 0x32, 0xcb,
	0x7c, 0x20, 0xfb, 0x21, 0x26, 0xff, 0x0e, 0x11, 0x3e,
	0x80, 0x32, 0xe2, 0x0c, 0x3e, 0xf3, 0xe2, 0x32, 0x3e,
	0x77, 0x77, 0x3e, 0xfc, 0xe0, 0x47, 0x11, 0x04, 0x01,
	0x21, 0x10, 0x80, 0x1a, 0xcd, 0x95, 0x00, 0xcd, 0x96,
	0x00, 0x13, 0x7b, 0xfe, 0x34, 0x20, 0xf3, 0x11, 0xd8,
	0x00, 0x06, 0x08, 0x1a, 0x13, 0x22, 0x23, 0x05, 0x20,
	0xf9, 0x3e, 0x19, 0xea, 0x10, 0x99, 0x21, 0x2f, 0x99,
	0x0e, 0x0c, 0x3d, 0x28, 0x08, 0x32, 0x0d, 0x20, 0xf9,
	0x2e, 0x0f, 0x18, 0xf3, 0x67, 0x3e, 0x64, 0x57, 0xe0,
	0x42, 0x3e, 0x91, 0xe0, 0x40, 0x04, 0x1e, 0x02, 0x0e,
	0x0c, 0xf0, 0x44, 0xfe, 0x90, 0x20, 0xfa, 0x0d, 0x20,
	0xf7, 0x1d, 0x20, 0xf2, 0x0e, 0x13, 0x24, 0x7c, 0x1e,
	0x83, 0xfe, 0x62, 0x28, 0x06, 0x1e, 0xc1, 0xfe, 0x64,
	0x20, 0x06, 0x7b, 0xe2, 0x0c, 0x3e, 0x87, 0xe2, 0xf0,
	0x42, 0x90, 0xe0, 0x42, 0x15, 0x20, 0xd2, 0x05, 0x20,
	0x4f, 0x16, 0x20, 0x18, 0xcb, 0x4f, 0x06, 0x04, 0xc5,
	0xcb, 0x11, 0x17, 0xc1, 0xcb, 0x11, 0x17, 0x05, 0x20,
	0xf5, 0x22, 0x23, 0x22, 0x23, 0xc9, 0xce, 0xed, 0x66,
	0x66, 0xcc, 0x0d, 0x00, 0x0b, 0x03, 0x73, 0x00, 0x83,
	0x00, 0x0c, 0x00, 0x0d, 0x00, 0x08, 0x11, 0x1f, 0x88,
	0x89, 0x00, 0x0e, 0xdc, 0xcc, 0x6e, 0xe6, 0xdd, 0xdd,
	0xd9, 0x99, 0xbb, 0xbb, 0x67, 0x63, 0x6e, 0x0e, 0xec,
	0xcc, 0xdd, 0xdc, 0x99, 0x9f, 0xbb, 0xb9, 0x33, 0x3e,
	0x3c, 0x42, 0xb9, 0xa5, 0xb9, 0xa5, 0x42, 0x3c, 0x21,
	0x04, 0x01, 0x11, 0xa8, 0x00, 0x1a, 0x13, 0xbe, 0x20,
	0xfe, 0x23, 0x7d, 0xfe, 0x34, 0x20, 0xf5, 0x06, 0x19,
	0x78, 0x86, 0x23, 0x05, 0x20, 0xfb, 0x86, 0x20, 0xfe,
	0x3e, 0x01, 0xe0, 0x50};

struct initval {
	unsigned short addr;
	unsigned char	val;
};

struct initval init_tab[] = {
	{0xFF05, 0x00},
	{0xFF06, 0x00},
	{0xFF07, 0x00},
	{0xFF0F, 0xE1},
	{0xFF10, 0x80},
	{0xFF11, 0xBF},
	{0xFF12, 0xF3},
	{0xFF14, 0xBF},
	{0xFF16, 0x3F},
	{0xFF17, 0x00},
	{0xFF19, 0xBF},
	{0xFF1A, 0x7F},
	{0xFF1B, 0xFF},
	{0xFF1C, 0x9F},
	{0xFF1E, 0xBF},
	{0xFF20, 0xFF},
	{0xFF21, 0x00},
	{0xFF22, 0x00},
	{0xFF23, 0xBF},
	{0xFF24, 0x77},
	{0xFF25, 0xF3},
	{0xFF26, 0xF1}, //0xF0 sgb
	{0xFF40, 0x91},
	{0xFF42, 0x00},
	{0xFF43, 0x00},
	{0xFF45, 0x00},
	{0xFF47, 0xFC},
	{0xFF48, 0xFF},
	{0xFF49, 0xFF},
	{0xFF4A, 0x00},
	{0xFF4B, 0x00},
	{0xFFFF, 0x00},
	{0x0000, 0x00}
};

unsigned char nlogo[] = {
	0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03,
	0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D, 0x00, 0x08,
	0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E,
	0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63,
	0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB,
	0xB9, 0x33, 0x3E};

struct iomask {
	unsigned char write;
	unsigned char read;
	unsigned char readonly;
};

//the below is a set of bitmasks used to emulate the
//behaviour of the actual registers in memory. Only
//certain bits in each can be read from or written to.

struct iomask _IOmasks[] = {
	{0xC0, 0x00, 0xC0}, //0xFF00 joypad (0x0F are read only)
	{0xFF, 0x00, 0xFF}, //0xFF01 serial
	{0x7E, 0x00, 0xFF}, //0xFF02 serial clock (0x7C cgb)
	{0xFF, 0x00, 0xFF}, //0xFF03 unused
	{0x00, 0x00, 0xFF}, //0xFF04 div (all writes set to 0)
	{0x00, 0x00, 0xFF}, //0xFF05 TIMA
	{0x00, 0x00, 0xFF}, //0xFF06 TMA
	{0xF8, 0x00, 0xFF}, //0xFF07 TAC
	{0xFF, 0x00, 0xFF}, //0xFF08 unused
	{0xFF, 0x00, 0xFF}, //0xFF09 unused
	{0xFF, 0x00, 0xFF}, //0xFF0A unused
	{0xFF, 0x00, 0xFF}, //0xFF0B unused
	{0xFF, 0x00, 0xFF}, //0xFF0C unused
	{0xFF, 0x00, 0xFF}, //0xFF0D unused
	{0xFF, 0x00, 0xFF}, //0xFF0E unused
	{0xE0, 0x00, 0xFF}, //0xFF0F IF
	{0x80, 0x00, 0xFF}, //0xFF10 NR10
	{0x00, 0x3F, 0xFF}, //0xFF11 NR11
	{0x00, 0x00, 0xFF}, //0xFF12 NR12
	{0x00, 0xFF, 0xFF}, //0xFF13 NR13 (readmask 0xFF)
	{0x38, 0xBF, 0xFF}, //0xFF14 NR14 (readmask 0xBF)
	{0xFF, 0x00, 0xFF}, //0xFF15 unused
	{0x00, 0x3F, 0xFF}, //0xFF16 NR21 (readmask 0x3F)
	{0x00, 0x00, 0xFF}, //0xFF17 NR22
	{0x00, 0x00, 0xFF}, //0xFF18 NR23
	{0x38, 0xBF, 0xFF}, //0xFF19 NR24 (readmask 0xBF)
	{0x7F, 0x7F, 0xFF}, //0xFF1A NR30 (readmask 0x7F)
	{0x00, 0x00, 0xFF}, //0xFF1B NR31
	{0x9F, 0x9F, 0xFF}, //0xFF1C NR32 (readmask 0x9F)
	{0x00, 0x00, 0xFF}, //0xFF1D NR33
	{0x38, 0xBF, 0xFF}, //0xFF1E NR34 (readmask 0xBF)
	{0xFF, 0x00, 0xFF}, //0xFF1F unused
	{0xC0, 0xC0, 0xFF}, //0xFF20 NR41 (readmask 0xC0)
	{0x00, 0x00, 0xFF}, //0xFF21 NR42
	{0x00, 0x00, 0xFF}, //0xFF22 NR43
	{0x3F, 0xBF, 0xFF}, //0xFF23 NR44 (readmask 0xBF)
	{0x00, 0x00, 0xFF}, //0xFF24 NR50
	{0x00, 0x00, 0xFF}, //0xFF25 NR51
	{0x70, 0x00, 0xF0}, //0xFF26 NR52 (0x0F are read only)
	{0xFF, 0x00, 0xFF}, //0xFF27 unused
	{0xFF, 0x00, 0xFF}, //0xFF28 unused
	{0xFF, 0x00, 0xFF}, //0xFF29 unused
	{0xFF, 0x00, 0xFF}, //0xFF2a unused
	{0xFF, 0x00, 0xFF}, //0xFF2b unused
	{0xFF, 0x00, 0xFF}, //0xFF2c unused
	{0xFF, 0x00, 0xFF}, //0xFF2d unused
	{0xFF, 0x00, 0xFF}, //0xFF2e unused
	{0xFF, 0x00, 0xFF}, //0xFF2f unused
	{0x00, 0x00, 0xFF}, //0xFF30 wave patterns
	{0x00, 0x00, 0xFF}, //0xFF31 wave patterns
	{0x00, 0x00, 0xFF}, //0xFF32 wave patterns
	{0x00, 0x00, 0xFF}, //0xFF33 wave patterns
	{0x00, 0x00, 0xFF}, //0xFF34 wave patterns
	{0x00, 0x00, 0xFF}, //0xFF35 wave patterns
	{0x00, 0x00, 0xFF}, //0xFF36 wave patterns
	{0x00, 0x00, 0xFF}, //0xFF37 wave patterns
	{0x00, 0x00, 0xFF}, //0xFF38 wave patterns
	{0x00, 0x00, 0xFF}, //0xFF39 wave patterns
	{0x00, 0x00, 0xFF}, //0xFF3A wave patterns
	{0x00, 0x00, 0xFF}, //0xFF3B wave patterns
	{0x00, 0x00, 0xFF}, //0xFF3C wave patterns
	{0x00, 0x00, 0xFF}, //0xFF3D wave patterns
	{0x00, 0x00, 0xFF}, //0xFF3E wave patterns
	{0x00, 0x00, 0xFF}, //0xFF3F wave patterns
	{0x00, 0x00, 0xFF}, //0xFF40 LCDC
	{0x80, 0x00, 0xF8}, //0xFF41 STAT (0x07 read only)
	{0x00, 0x00, 0xFF}, //0xFF42 SCY
	{0x00, 0x00, 0xFF}, //0xFF43 SCX
	{0x00, 0x00, 0xFF}, //0xFF44 LY
	{0x00, 0x00, 0xFF}, //0xFF45 LYC
	{0x00, 0x00, 0xFF}, //0xFF46 DMA transfer
	{0x00, 0x00, 0xFF}, //0xFF47 BGP
	{0x00, 0x00, 0xFF}, //0xFF48 OBP0 (non cgb only) (0x03 not used)
	{0x00, 0x00, 0xFF}, //0xFF49 OBP1 (non cgb only) (0x03 not used)
	{0x00, 0x00, 0xFF}, //0xFF4A WY
	{0x00, 0x00, 0xFF}, //0xFF4B WX (minus 7)
	{0xFF, 0x00, 0xFF}, //0xFF4C unused
	{0xFF, 0x00, 0xFF}, //0xFF4D cgb key 1 (write 0x3F read 0x3F 0x80 read only)
	{0xFF, 0x00, 0xFF}, //0xFF4E unused
	{0xFF, 0x00, 0xFF}, //0xFF4F cgb vram bank (write 0xFE, read 0xFE)
	{0x00, 0x00, 0xFF}, //0xFF50 gb bootrom disable
	{0x00, 0x00, 0xFF}, //0xFF51 cgb HDMA1
	{0x00, 0x00, 0xFF}, //0xFF52 cgb HDMA2
	{0x00, 0x00, 0xFF}, //0xFF53 cgb HDMA3
	{0x00, 0x00, 0xFF}, //0xFF54 cgb HDMA4
	{0x00, 0x00, 0xFF}, //0xFF55 cgb HDMA5
	{0xFF, 0x00, 0xFF}, //0xFF56 cgb IR com (write 0x3C read 0x3C 0x02 read only)
	{0xFF, 0x00, 0xFF}, //0xFF57 unused
	{0xFF, 0x00, 0xFF}, //0xFF58 unused
	{0xFF, 0x00, 0xFF}, //0xFF59 unused
	{0xFF, 0x00, 0xFF}, //0xFF5A unused
	{0xFF, 0x00, 0xFF}, //0xFF5B unused
	{0xFF, 0x00, 0xFF}, //0xFF5C unused
	{0xFF, 0x00, 0xFF}, //0xFF5D unused
	{0xFF, 0x00, 0xFF}, //0xFF5E unused
	{0xFF, 0x00, 0xFF}, //0xFF5F unused
	{0xFF, 0x00, 0xFF}, //0xFF60 unused
	{0xFF, 0x00, 0xFF}, //0xFF61 unused
	{0xFF, 0x00, 0xFF}, //0xFF62 unused
	{0xFF, 0x00, 0xFF}, //0xFF63 unused
	{0xFF, 0x00, 0xFF}, //0xFF64 unused
	{0xFF, 0x00, 0xFF}, //0xFF65 unused
	{0xFF, 0x00, 0xFF}, //0xFF66 unused
	{0xFF, 0x00, 0xFF}, //0xFF67 unused
	{0xFF, 0x00, 0xFF}, //0xFF68 cgb BCPS/BGPI (write 0x40)
	{0xFF, 0x00, 0xFF}, //0xFF69 cgb BCPD/BGPD (complicated af)
	{0xFF, 0x00, 0xFF}, //0xFF6A cgb OCPS/OBPI (same as above)
	{0xFF, 0x00, 0xFF}, //0xFF6B cgb OCPD/OBPD (same as above)
	{0xFF, 0x00, 0xFF}, //0xFF6C cgb undocumented (write 0xFE)
	{0xFF, 0x00, 0xFF}, //0xFF6D unused
	{0xFF, 0x00, 0xFF}, //0xFF6E unused
	{0xFF, 0x00, 0xFF}, //0xFF6F unused
	{0xF8, 0x00, 0xFF}, //0xFF70 cgb wram bank (write 00 is 01)
	{0xFF, 0x00, 0xFF}, //0xFF71 unused
	{0x00, 0x00, 0xFF}, //0xFF72 undocumented
	{0x00, 0x00, 0xFF}, //0xFF73 undocumented
	{0xFF, 0x00, 0xFF}, //0xFF74 cgb undocumented (write 0x00)
	{0x8F, 0x00, 0xFF}, //0xFF75 undocumented
	{0x00, 0x00, 0xFF}, //0xFF76 always 0x00
	{0x00, 0x00, 0xFF}, //0xFF77 always 0x00
	{0xFF, 0x00, 0xFF}, //0xFF78 unused
	{0xFF, 0x00, 0xFF}, //0xFF79 unused
	{0xFF, 0x00, 0xFF}, //0xFF7A unused
	{0xFF, 0x00, 0xFF}, //0xFF7B unused
	{0xFF, 0x00, 0xFF}, //0xFF7C unused
	{0xFF, 0x00, 0xFF}, //0xFF7D unused
	{0xFF, 0x00, 0xFF}, //0xFF7E unused
	{0xFF, 0x00, 0xFF}, //0xFF7F unused
};

mmu::mmu(unsigned char type)
{
	_cgb_mode = 0;//set to 0x03 for easy bank switch
	_vram.resize(2);//handle cgb banks later
	_wram1.resize(7);//handle cgb banks later
	_IOReg[0x50] = 0;
	vramWrite = false;
	_oamtime = 0;
	for (unsigned char io = 0; io < 0x7F; io++)
		_IOReg[io] = _IOmasks[io].write;
	_IOReg[0x00] |= 0x0F;
	_clock = 0;
	_tac0 = 0;
	(void)type;
// TODO: factory method to generate the different mmus 
}
		//Bit values for each button when polling input
		//20 10 (toggles between reading the dpad and reading the buttons
		//8 down/start
		//4 up/select
		//2 left/b
		//1 right/a
//UNKNOWN: can the console be set to read from both sets of buttons at once?

void mmu::pollInput(void)
{
	_IOReg[0x00] |= 0x0F;
	const unsigned char *keystat = SDL_GetKeyboardState(NULL);
	unsigned char res = 0;
	if (_IOReg[0x00] & 0x20)
	{
		if (keystat[SDL_SCANCODE_DOWN])
			res |= 0x08;
		if (keystat[SDL_SCANCODE_UP])
			res |= 0x04;
		if (keystat[SDL_SCANCODE_LEFT])
			res |= 0x02;
		if (keystat[SDL_SCANCODE_RIGHT])
			res |= 0x01;
	}
	else if (_IOReg[0x00] & 0x10)
	{
		if (keystat[SDL_SCANCODE_RETURN])
			res |= 0x08;
		if (keystat[SDL_SCANCODE_RSHIFT])
			res |= 0x04;
		if (keystat[SDL_SCANCODE_X])
			res |= 0x02;
		if (keystat[SDL_SCANCODE_Z])
			res |= 0x01;
	}
	_IOReg[0x00] ^= res;
}

//enables the bootrom then passes cartridge loading to the
//cart class's factory method.

void mmu::loadCart(char *filename)
{
	cart _tmp;
	_IOReg[0x50] = 0xFE;
	_cart = _tmp.loadCart(filename);
}

//bit values for each type of interrupt:
//1 VBlank 2 STAT 4 Timer 8 Serial 16 Joypad
//VBlank happens beginning of every VBlank
//STAT triggers on conditions set in STAT reg
//Timer triggers on a frequency determined by a set of registers
////when 0xFF05 goes from FF to 00
//Serial triggers when a byte is received via serial port
//Joypad triggers on button press

void	mmu::setINTS(void)
{
	if (!(_IOReg[0x40] & (1 << 7)))
		return ;
	unsigned char LY = _IOReg[0x44];
	unsigned char LYC = _IOReg[0x45];
	unsigned char STAT = _IOReg[0x41];
	unsigned char mode = STAT & 0x03;
	if (LY == LYC)
		_IOReg[0x41] |= (1 << 2);
	else
		_IOReg[0x41] &= ~(1 << 2);
	if (((LY == LYC && _IOReg[0x41] & (1 << 6)) || (mode == 2 && _IOReg[0x41] & (1 << 5)) || (mode == 1 && (_IOReg[0x41] & (1 << 4) || _IOReg[0x41] & (1 << 5))) || (mode == 0 && _IOReg[0x41] & (1 << 3))))
		_IOReg[0x0F] |= 0x02;
	if ((_IOReg[0x00] & 0x0F) != 0x0F) //joypad interrupt
		_IOReg[0x0F] |= (1 << 4);
	if (_IOReg[0x44] == 144)
		_IOReg[0x0F] |= 1;
}

//STAT is 0xFF41. Its 3 lower bits determine the current PPU mode.
//During mode 0 and mode 1 the CPU can access both VRAM and OAM. During mode 2 the CPU
//can only access VRAM, not OAM. During mode 3 OAM and VRAM can't be accessed. In GBC
//mode the CPU can't access Palette RAM (FF69h and FF6Bh) during mode 3

//this function allows the PPU to access all regions of memory. Only to be used
//in the PPU source code. Written as a nearly identical version of accessAt to
//save on cpu instructions related to checking if we're accessing from the ppu
//or not
unsigned char	mmu::PaccessAt(unsigned short addr)
{
//	PRINT_DEBUG("PAccessing 0x%04x", addr);
	unsigned char	val = 0xFF;
	if (addr <= 0x7FFF || (0xA000 <= addr && addr <= 0xBFFF))
    {
        if ((addr < 0x100) && ! (_IOReg[0x50] & 0x01))
            val = _booter[addr];
        else
	    	val = _cart->readFrom(addr);
    }
	if (0xE000 <= addr && addr <= 0xFDFF)
		val = 0xFF;
	else if (0x8000 <= addr && addr <= 0x9FFF)
		val = _vram[_IOReg[0x4F] & 1][addr - 0x8000];
	else if (0xC000 <= addr && addr <= 0xCFFF)
		val = _wram0[addr - 0xC000];
	else if (0xD000 <= addr && addr <= 0xDFFF)
		val = _wram1[_IOReg[0x70] & _cgb_mode][addr - 0xD000];
	else if (0xE000 <= addr && addr <= 0xFDFF)
		val = PaccessAt(addr - 0x2000);
	else if (0xFE00 <= addr && addr <= 0xFE9F)
		val = _oam[addr - 0xFE00];
	else if (0xFEA0 <= addr && addr <= 0xFEFF)//unused
		val = 0xFF; //changes for cgb
	else if (0xFF00 <= addr && addr <= 0xFF7F)
		val = _IOReg[addr - 0xFF00];
	else if (0xFF80 <= addr && addr <= 0xFFFE)
		val = _hram[addr - 0xFF80];
	else if (addr == 0xFFFF)
		val = _IE;
//	PRINT_DEBUG("Returning: 0x%02x", val);
	return val;
}

//This function is for the CPU to access memory. Includes memory protection
//for modes 2 and 3
//To be used in the CPU source code.
unsigned char	mmu::accessAt(unsigned short addr)
{
	unsigned char	val = 0xFF;
	if (_oamtime && (addr < 0xFF80 || addr == 0xFFFF))
		return val;
	if (addr <= 0x7FFF || (0xA000 <= addr && addr <= 0xBFFF))
	{
//		PRINT_DEBUG("Accessing 0x%04x", addr);
//		PRINT_DEBUG("0x%hx < 0x100 (%u) && 0xFF50 (0x%hhx)", addr, addr < 0x100, _IOReg[0x50]);
		if ((addr < 0x100) && !(_IOReg[0x50] & 0x01))
			val = _booter[addr];
		else
			val = _cart->readFrom(addr);
		//val = ((addr < 0x100) && (_IOReg[0x50] == 0x01)) ?_cart->readFrom(addr) : _booter[addr];
	}
	else if (0x8000 <= addr && addr <= 0x9FFF)
	{
		if ((_IOReg[0x41] & 0x03) < 0x03)
			val = _vram[_IOReg[0x4F] & 1][addr - 0x8000];
		else
			printf("Bad vram access\n");
	}
	else if (0xC000 <= addr && addr <= 0xCFFF)
		val = _wram0[addr - 0xC000];
	else if (0xD000 <= addr && addr <= 0xDFFF)
		val = _wram1[_IOReg[0x70] & _cgb_mode][addr - 0xD000];
	else if (0xE000 <= addr && addr <= 0xFDFF)
		val = accessAt(addr - 0x2000);
	else if (0xFE00 <= addr && addr <= 0xFE9F)
	{
		if (!_oamtime && (_IOReg[0x41] & 0x03) < 2)
			val = _oam[addr - 0xFE00];
		else
			printf("Bad OAM access\n");
	}
	else if (0xFEA0 <= addr && addr <= 0xFEFF)//unused
		val = 0xFF; //changes for cgb
	else if (0xFF00 <= addr && addr <= 0xFF7F)
		val = _IOReg[addr - 0xFF00] | _IOmasks[addr - 0xFF00].read;
	else if (0xFF80 <= addr && addr <= 0xFFFE)
		val = _hram[addr - 0xFF80];
	else if (addr == 0xFFFF)
		val = _IE;
//	PRINT_DEBUG("Returning: 0x%02x", val);
	return val;
}

//CGB: There are another 32 bytes at FEA0h – FEBFh.
//At FEC0h – FECFh there are another 16
//bytes that are repeated in FED0h – FEDFh, FEE0h – FEEFh
//and FEF0h – FEFFh. Reading and
//writing to any of this 4 blocks will change the same
//16 bytes. This is true for revision D of the GBC
//CPU.
//
//unreadable IO register bits return 1
//
//write a function to fail writes to unwritable registers
//
//when implementing cgb, implement hdma1-hdma5

//Used by the PPU to update the mode the STAT register shows
//
void	mmu::STATupdate(unsigned char mode)
{
	unsigned char _stat = _IOReg[0x41];
	_stat = (_stat & 0xF8) | (mode & 0x03);
	_IOReg[0x41] = _stat;
}

//Used to write to the IO registers using the bitmasks above

void	mmu::_IOwrite(unsigned short addr, unsigned char msg)
{
//	PRINT_DEBUG("IOwrite 0x%04X 0x%02X", addr, msg);
	unsigned char mode;
	addr -= 0xFF00;
	if (addr == 0x41)
		mode = _IOReg[0x41] & 0x03;
	_IOReg[addr] = (msg | _IOmasks[addr].write) & _IOmasks[addr].readonly;
	if (addr == 0x41)
		_IOReg[0x41] |= mode;
	if (!addr)
		_IOReg[0x00] |= msg;
	if (addr == 0x46 && (_IOReg[0x41] & 0x03) < 0x02)
		_oamtime = 640;
}

//Allows the CPU to write to areas in memory. Has memory protections in place
//for PPU modes 2 and 3

void	mmu::writeTo(unsigned short addr, unsigned char msg)
{
	printf("write 0x%02hhx to 0x%04hx\n", msg, addr);
	if ((_IOReg[0x50] & 1) && (addr <= 0x7FFF || (0xA000 <= addr && addr <= 0xBFFF)))
		_cart->writeTo(addr, msg);
	else if (0x8000 <= addr && addr <= 0x9FFF)
	{
		if ((_IOReg[0x41] & 0x03) < 0x03)
		{
			_vram[_IOReg[0x4F] & 1][addr - 0x8000] = msg;
			vramWrite = true;
		}
		else
			PRINT_DEBUG("Bad vram access\n");
	}
	else if (0xC000 <= addr && addr <= 0xCFFF)
		_wram0[addr - 0xC000] = msg;
	else if (0xD000 <= addr && addr <= 0xDFFF)
		_wram1[_IOReg[0x70] & _cgb_mode][addr - 0xD000] = msg;
	else if (0xE000 <= addr && addr <= 0xFDFF)
		writeTo(addr - 0x2000, msg);
	else if (0xFE00 <= addr && addr <= 0xFE9F)//check ppu mode
	{
		if ((_IOReg[0x41] & 0x03) < 0x02)
			_oam[addr - 0xFE00] = msg;
		else
			printf("Bad OAM access\n");
	}
	else if (0xFEA0 <= addr && addr <= 0xFEFF)//unused
		return ;
	else if (0xFF00 <= addr && addr <= 0xFF7F)
	{
//		PRINT_DEBUG("_IOReg at 0x%p", _IOReg);
		_IOwrite(addr, msg);
	}
	else if (0xFF80 <= addr && addr <= 0xFFFE)
		_hram[addr - 0xFF80] = msg;
	else if (addr == 0xFFFF)
		_IE = msg;
	printf("wrote: 0x%02hhx\n", accessAt(addr));
}

// go back and handle obscure timer behaviour
// cpu speed 4194304
// TODO: figure out what bug is causing the timer interrupt to not trigger properly
void	mmu::timerInc(unsigned cycles)
{
	//FF04 DIV increments 16384 times a second (every 256 cycles, affected by double speed)
	//FF05 TIMA increments by specified TAC frequency (load from TMA and interrupt when overflow)
	//FF06 TMA loads into TIMA on overflow
	//FF07 TAC bit 2 start/stop (div always counts)
	// bits 1+0:
	//	00: 4096 (every 1024 cycles)
	//	01: 262144 (every 16 cycles)
	//	10: 65536 (every 64 cycles)
	//	11: 16384 (every 256 cycles)
	unsigned char tac = _IOReg[0x07];
	unsigned timatab[]= {
		1024, 16, 64, 256};
	_clock += cycles;
	_tac0 += cycles;
	if (_clock >= 256)
	{
		_IOReg[0x04] += 1;
//		PRINT_DEBUG("Clock inc %u", _IOReg[0x04]);
//		PRINT_DEBUG("tac %X", tac);
		_clock -= 256;
	}
	if (!(tac & (1 << 2)))
		return ;
	tac &= 0x03;
//	printf("timatab %u", tac);
	if (_tac0 >= timatab[tac])
	{
		if (_IOReg[0x05] == 0xFF)
		{
			_IOReg[0x0F] |= (1 << 2);
			_IOReg[0x05] = _IOReg[0x06];
//			PRINT_DEBUG("INT set");
		}
		else
		{
//			PRINT_DEBUG("Timer inc");
			_IOReg[0x05] += 1;
		}
		while (_tac0 >= timatab[tac])
			_tac0 -= timatab[tac];
	}
}
