#include "cart.hpp"

cart::cart()
{
	return ;
}

//construct a barebones cart with the absolute minimum
//required memory space. Fill the first two banks with
//ROM data
cart::cart(FILE *rom, unsigned short ram)
{
	_romSpace.resize(0x02);
	_ramSpace.resize(0x00);
	fread(&_romSpace[0x00][0x00], 1, 0x4000, rom);
	fread(&_romSpace[0x01][0x00], 1, 0x4000, rom);
	if (ram)
		_ramSpace.resize(ram);
	return;
}

//uses a byte from the ROM's header to determine the
//actual size used by the game so _romSpace can be
//resized if needed
unsigned short 	cart::romSizetab(unsigned char size)
{
	switch(size)
	{
		case 0x00: //32KB no banking/2banks
			return 0x02;
		case 0x01: //64KB 4 banks
			return 0x04;
		case 0x02://128KB 8 banks
			return 0x08;
		case 0x03: //256KB 16 banks
			return 0x10;
		case 0x04: //512KB 32 banks
			return 0x20;
		case 0x05: //1MB 64 banks
			return 0x40;
		case 0x06: //2MB 128 banks
			return 0x800;
		case 0x07: //4MB 256 banks
			return 0x100;
		case 0x08:
			return 0x200;
		case 0x09 ... 0x51:
			return 0x00;
		case 0x52: //1.1MB 72 banks
			return 0x48;
		case 0x53: //1.2MB 80 banks
			return 0x50;
		case 0x54: //1.5MB 96 banks
			return 0x60;
	}
	return 0x00;
}

//uses a byte from the ROM's header to determine the
//amount of RAM that it needs so _ramSpace can be resized
//if needed
unsigned short	cart::ramSizetab(unsigned char size)
{
	switch(size)
	{
		case 0x00: //none
			_ramSize = 0x00;
			return 0x00;
		case 0x01: //2KB
			_ramSize = 0x800;
			return 0x01;
		case 0x02: //8KB
			_ramSize = 0x2000;
			return 0x01;
		case 0x03: //32KB 4 banks
			_ramSize = 0x8000;
			return 0x04;
		case 0x04: //128KB 16 banks
			_ramSize = 0x20000;
			return 0x10;
		case 0x05: //64KB 8 banks
			_ramSize = 0x10000;
			return 0x08;
	}
	return 0x00;
}

//factory method used to generate the type of cartridge
//used by the rom. determines ramsize, romsize, and MBC
//based on bytes located in the ROM header
std::unique_ptr<cart>	cart::loadCart(char *fname)
{
	if (!fname)
		throw "Error: No cart name";
	//cart reading. 0x147 mbc 0x148 romsize 0x149 ramsize
	unsigned char	header[0x14F];
	FILE *rom = fopen(fname, "r");
	fseek(rom, 0, SEEK_END);
	_romSize = ftell(rom);
	if (_romSize < 0x14F)
	{
		fclose(rom);
		throw "Error: cart size (too small)";
	}
	rewind(rom);
	fread(header, 1, 0x14F, rom);
	rewind(rom);
	GBbit = header[0x143];
	printf("Cart %.16s\n", &header[0x134]);
	printf("MBC 0x%02hhx Romsize byte 0x%02hhx Ramsize byte 0x%02hhx\n\n", header[0x147], header[0x148], header[0x149]);
	switch (header[0x147])
	{
		case 0x00: //ROM ONLY
			return std::make_unique<cart>(cart(rom, 0));
		case 0x01 ... 0x03: //MBC1
			return std::make_unique<mbc1>(mbc1(romSizetab(header[0x148]), ramSizetab(header[0x149]), rom));
		case 0x04:
			break ;
		case 0x05 ... 0x06: //MBC2
			return std::make_unique<mbc2>(mbc2(romSizetab(header[0x148]), ramSizetab(header[0x149]), rom));
		case 0x07:
			break;
		case 0x08 ... 0x09: //ROM+RAM/ROM+RAM+BATTERY
			return std::make_unique<cart>(cart(rom, ramSizetab(header[0x149])));
		case 0x0A:
			break;
		case 0x0B ... 0x0D: //MMM01/MMM01+RAM/MMM01+RAM+BATTERY
//			return mmm01(romSizetab(header[0x148]), ramSizetab(header[0x149]), rom);
		case 0x0E:
			break;
		case 0x0F ... 0x13: //MBC3+TIMER+BATTERY/MBC3+TIMER+RAM+BATTERY/MBC3/MBC3+RAM/MBC3+RAM+BATTERY
			return std::make_unique<mbc3>(mbc3(romSizetab(header[0x148]), ramSizetab(header[0x149]), rom));
		case 0x14 ... 0x18:
			break;
		case 0x19 ... 0x1E: //MBC5/MBC5+RAM/MBC5+RAM+BATTERY/MBC5+RUMBLE/MBC5+RUMBLE+RAM/MBC5+RUMBLE+RAM+BATTERY
			return std::make_unique<mbc5>(mbc5(romSizetab(header[0x148]), ramSizetab(header[0x149]), rom));
		case 0x20: //MBC6
//			return mbc6(romSizetab(header[0x148]), ramSizetab(header[0x149]), rom);
		case 0x22: //MBC7+SENSOR+RUMBLE+RAM+BATTERY
//			return mbc7(romSizetab(header[0x148]), ramSizetab(header[0x149]), rom);

				//get to these way later
			//MM01, MBC1-M, MBC6, MBC7 still need implementing
//		0xFC //POCKET CAMERA
//		0xFD //BANDAI TAMA5
//		0xFE //HuC3
//		0xFF //HuC1+RAM+BATTERY
		break;
	}
	throw "Error: unsupported MBC";
	return NULL;
}

void	cart::writeTo(unsigned short addr, unsigned char val)
{
	return ;
}

//no official carts have ram with no mbc
unsigned char	cart::readFrom(unsigned short addr)
{
	if (0x0000 <= addr && addr <= 0x3FFF)
		return (_romSpace[0x00][addr]);
	else if (0x4000 <= addr && addr <= 0x7FFF)
		return (_romSpace[0x01][addr - 0x4000]);
	else if (_ramSpace.size() && 0xA000 <= addr && addr <= 0xBFFF)
		return _ramSpace[0x00][addr - 0xA000];
	return (0xFF);
}

mbc1::mbc1(unsigned romBanks, unsigned ramBanks, FILE *rom)
{
	printf("MBC1 rom %u ram %u\n", romBanks, ramBanks);
	_romSpace.resize(romBanks ? romBanks : 2);
	_ramSpace.resize(ramBanks);
	for (unsigned i = 0x00; i < romBanks; i++)
		fread(&_romSpace[i][0], 1, 0x4000, rom);
	_ramg = 0;
	_bank1 = 1;
	_bank2 = 0;
	_mode = 0;
	fclose(rom);
}

void			mbc1::writeTo(unsigned short addr, unsigned char val)
{
	printf("write 0x%02hhx to 0x%04hx\n\n", val, addr);
	if (0x0000 <= addr && addr <= 0x1FFF)
		_ramg = val & 0x0F;
	else if (0x2000 <= addr && addr <= 0x3FFF)
		_bank1 = (val & 0x1F) ? (val & 0x1F) : 1;
	else if (0x4000 <= addr && addr <= 0x5FFF)
		_bank2 = ((val >> 5) & 0x03);
	else if (0x6000 <= addr && addr <= 0x7FFF)
		_mode = (val & 0x01);
	else if (_ramSize && 0xA000 <= addr && addr <= 0xBFFF)
		_ramWrite(addr - 0xA000, val);
}
//if shared pointers work how I hope and I decide to shove
//a bunch of pointers into a bigger struct then I can just
//direct read the given address
//if not, I need to check if I'm in the upper half of rom
//and jump ahead 16KB * rom bank number
//	0x4000 = 16KB 0x2000 = 8KB
//
//	verify behaviour when an address is larger than the
//	given rom/ram space

void		mbc1::_ramWrite(unsigned short addr, unsigned char val)
{
	printf("ffffffffffff\n");
	if ((_ramg & 0x0F) != 0x0A)
		return ;
	unsigned char banknum = 0;
	if (_ramSize > 0x2000)
		banknum = _mode ? _bank2 : 0;
	printf("banknum %u\n", banknum);
	_ramSpace[banknum][addr] = val;
	if (_ramSize < 0x2000)
	{
		addr += _ramSize;
		while (addr < 0x2000)
		{
			_ramSpace[banknum][addr] = val;
			addr += _ramSize;
		}
	}
}

unsigned char	mbc1::_rombankRead(unsigned short addr)
{
	unsigned char banknum = 0;
	if (addr >= 0x4000)
	{
		banknum = _bank1 | (!_mode ? (_bank2 << 5) : 0);
		addr -= 0x4000;
	}
	return (_romSpace[banknum][addr]);
}

unsigned char	mbc1::_rambankRead(unsigned short addr)
{
//	printf("we in here?\n");
	unsigned char banknum = 0;
	if ((_ramg & 0x0F) != 0x0A)
		return (0xFF);
	if (_ramSize > 0x2000)
		banknum = _mode ? _bank2 : 0;
//	printf("banknum 0x%02hhx\n", banknum);
	return (_ramSpace[banknum][addr]);
}

unsigned char	mbc1::readFrom(unsigned short addr)
{
//	printf("we in here 0x%04x\n", addr);
	if (0x0000 <= addr && addr <= 0x7FFF)
		return(_rombankRead(addr));
	else if (0xA000 <= addr && addr <= 0xBFFF)
		return (_rambankRead(addr - 0xA000));
	return 0xFF;
}

mbc2::mbc2(unsigned romBanks, unsigned ramBanks, FILE *rom)
{
	printf("MBC2\n");
	_romSpace.resize(romBanks ? romBanks : 2);
	_ramSize = 0x200;
	for (unsigned i = 0x00; i < romBanks; i++)
		fread(&_romSpace[i][0], 1, 0x4000, rom);
	fclose(rom);
	_ramg = 0;
	_romg = 1;
//	for (unsigned i = 0x00; i < romBanks; i++)
//		for (unsigned j = 0x00; j < 0x4000; j++)
//			printf("%c", _romSpace[i][j]);
}

void	mbc2::writeTo(unsigned short addr, unsigned char val)
{
	if (0x0000 <= addr && addr <= 0x3FFF)
	{
		if ((0x0F00 & addr) == 0x0100)
			_ramg = val;
		else
			_romg = val ? val & 0x0F: 1;
	}
	else if (_ramSize && 0xA000 <= addr && addr <= 0xBFFF)
		_ramWrite(addr - 0xA000, val);
}

unsigned char	mbc2::readFrom(unsigned short addr)
{
	if (0x0000 <= addr && addr <= 0x7FFF)
		_rombankRead(addr);
	else if (0xA000 <= addr && addr <= 0xBFFF)
		_ramRead(addr - 0xA000);
	return 0xFF;
}

void	mbc2::_ramWrite(unsigned short addr, unsigned char val)
{
	if ((_ramg & 0x0F) != 0x0A)
		return ;
	while (addr > _ramSize)
		addr -= _ramSize;
	while (addr <= 0x2000)
	{
		_ramSpace[0x00][addr] = val & 0x0F;
		addr += _ramSize;
	}
}

unsigned char	mbc2::_rombankRead(unsigned short addr)
{
	if (0x0000 <= addr && addr <= 0x3FFF)
		return (_romSpace[0x00][addr]);
	return (_romSpace[_romg][addr - 0x4000]);
}

unsigned char	mbc2::_ramRead(unsigned short addr)
{
	if ((_ramg & 0x0F) != 0x0A)
		return (0xFF);
	else
		return (_ramSpace[0x00][addr]);
}

mbc3::mbc3(unsigned romBanks, unsigned ramBanks, FILE *rom)
{
	printf("MBC3\n");
	_romSpace.resize(romBanks ? romBanks : romBanks + 2);
	_ramSpace.resize(ramBanks);
	_ramg = 0;
	_rombank = 1;
	_rambank = 0;
	_timeLatch = 0xFF;
	_rtcS = 0;
	_rtcM = 0;
	_rtcH = 0;
	_rtcDL = 0;
	_rtcDH = 0;
	for (unsigned i = 0x00; i < romBanks; i++)
		fread(&_romSpace[i][0], 1, 0x4000, rom);
	fclose(rom);
}

void			mbc3::writeTo(unsigned short addr, unsigned char val)
{
	if (0x0000 <= addr && addr <= 0x1FFF)
		_ramg = val;
	else if (0x2000 <= addr && addr <= 0x3FFF)
		_rombank = val & 0x7F ? val : 1;
	else if (0x4000 <= addr && addr <= 0x5FFF)
		_rambank = val;
	else if (0x6000 <= addr && addr <= 0x7FFF)
		_latchTime(val);
	else if (_ramSize && 0xA000 <= addr && addr <= 0xBFFF)
		_ramWrite(addr - 0xA000, val);
}

void		mbc3::_latchTime(unsigned char val)
{
	if (val == 0x01 && _timeLatch == 0x00)
	{
		_rtcS = 1;
		_rtcM = 1;
		_rtcH = 1;
		_rtcDL = 1;
		_rtcDH = 1;
		//set time registers to system clock
	}
	_timeLatch = val;
}

void		mbc3::_ramWrite(unsigned short addr, unsigned char val)
{
	if (_ramg != 0x0A)
		return ;
	unsigned char banknum = 0;
	if (_ramSize > 0x2000)
		banknum = _rambank;
	_ramSpace[banknum][addr] = val;
	if (_ramSize < 0x2000)
	{
		addr += _ramSize;
		while (addr < 0x2000)
		{
			_ramSpace[banknum][addr] = val;
			addr += _ramSize;
		}
	}
}

unsigned char	mbc3::_rombankRead(unsigned short addr)
{
	unsigned char banknum = 0;
	if (addr >= 0x4000)
	{
		banknum = _rombank;
		addr -= 0x4000;
	}
	return (_romSpace[banknum][addr]);
}

unsigned char	mbc3::_rambankRead(unsigned short addr)
{
	if ((_ramg & 0x0F) != 0x0A)
		return (0xFF);
	if (_rambank < 0x08)
		return (_ramSpace[_rambank][addr]);
	else
		switch (_rambank)
		{
			case 0x00 ... 0x08:
				return _rtcS;
			case 0x09:
				return _rtcM;
			case 0x0A:
				return _rtcH;
			case 0x0B:
				return _rtcDL;
			case 0x0C:
				return _rtcDH;
		}
	return 0xFF;
}

unsigned char	mbc3::readFrom(unsigned short addr)
{
	if (0x0000 <= addr && addr <= 0x7FFF)
		return(_rombankRead(addr));
	else if (0xA000 <= addr && addr <= 0xBFFF)
		return (_rambankRead(addr - 0xA000));
	return 0xFF;
}

mbc5::mbc5(unsigned romBanks, unsigned ramBanks, FILE *rom)
{
	printf("MBC5\n");
	_romSpace.resize(romBanks ? romBanks : 2);
	_ramSpace.resize(ramBanks);
	for (unsigned i = 0x00; i < romBanks; i++)
		fread(&_romSpace[i][0], 1, 0x4000, rom);
	_ramg = 0;
	_bank1 = 0;
	_bank2 = 0;
	_rambank = 0;
	fclose(rom);
}

#include <stdlib.h>
void			mbc5::writeTo(unsigned short addr, unsigned char val)
{
	if (0x0000 <= addr && addr <= 0x1FFF)
//		_ramg = val & 0x0F;
		_ramg = val;
	else if (0x2000 <= addr && addr <= 0x2FFF)
		_bank1 = val;
	else if (0x3000 <= addr && addr <= 0x3FFF)
		_bank2 = val & 0x01;
	else if (0x4000 <= addr && addr <= 0x5FFF)
		_rambank = val;
	else if (_ramSize && 0xA000 <= addr && addr <= 0xBFFF)
		_ramWrite(addr - 0xA000, val);
}

void		mbc5::_ramWrite(unsigned short addr, unsigned char val)
{
	if ((_ramg & 0x0F) != 0x0A)
		return ;
	unsigned char banknum = 0;
	if (_ramSize > 0x2000)
		banknum = _rambank;
	printf("ram bank %u\n", _rambank);
	_ramSpace[banknum][addr] = val;
	if (_ramSize < 0x2000)
	{
		addr += _ramSize;
		while (addr < 0x2000)
		{
			_ramSpace[banknum][addr] = val;
			addr += _ramSize;
		}
	}
}

unsigned char	mbc5::_rombankRead(unsigned short addr)
{
	unsigned short banknum = 0;
	if (addr >= 0x4000)
	{
		banknum = _bank1 | (_bank2 ? 0x0100 : 0x0000);
		addr -= 0x4000;
	}
	return (_romSpace[banknum][addr]);
}

unsigned char	mbc5::_rambankRead(unsigned short addr)
{
	unsigned char banknum = 0;
//	if ((_ramg & 0x0F) != 0x0A)
	if (_ramg != 0x0A)
		return (0xFF);
	if (_ramSize > 0x2000)
		banknum = _rambank;
	return (_ramSpace[banknum][addr]);
}

unsigned char	mbc5::readFrom(unsigned short addr)
{
	if (0x0000 <= addr && addr <= 0x7FFF)
		return(_rombankRead(addr));
	else if (0xA000 <= addr && addr <= 0xBFFF)
		return (_rambankRead(addr - 0xA000));
	return 0xFF;
}
