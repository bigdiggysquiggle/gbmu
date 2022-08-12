#include "print_debug.hpp"
#include "cart.hpp"
uint8_t cart::GBbit;
uint32_t cart::_romBanks;
uint32_t cart::_ramBanks;
uint32_t cart::_ramSize;

cart::cart()
{
	return ;
}

//construct a barebones cart with the absolute minimum
//required memory space. Fill the first two banks with
//ROM data
cart::cart(FILE *rom, uint16_t ram)
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
uint16_t 	cart::romSizetab(uint8_t size)
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
			return 0x80;
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
uint16_t	cart::ramSizetab(uint8_t size)
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
	uint8_t	header[0x14F];
	FILE *rom = fopen(fname, "r");
	if (rom == NULL)
		throw "Error: invalid rom file";
	fseek(rom, 0, SEEK_END);
	_romBanks = ftell(rom);//_romBanks is convenient and is going to be used to reflect the actual rom bank count later
	if (_romBanks < 0x14F)
	{
		fclose(rom);
		throw "Error: cart size (too small)";
	}
	rewind(rom);
	fread(header, 1, 0x14F, rom);
	rewind(rom);
	GBbit = header[0x143];
	printf("Cart %.16s\n", &header[0x134]);
	printf("MBC 0x%02hhx Romsize byte 0x%02hhx Ramsize byte 0x%02hhx\n", header[0x147], header[0x148], header[0x149]);
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

//this is a dummy function so that carts that don't have an MBC don't break things if
//they attempt to write values to ROM space
void	cart::writeTo(uint16_t addr, uint8_t val)
{
	(void)addr;
	(void)val;
	return ;
}

//no official carts have ram with no mbc (allegedly)
uint8_t	cart::readFrom(uint16_t addr)
{
	if (addr <= 0x3FFF)
		return (_romSpace[0x00][addr]);
	else if (0x4000 <= addr && addr <= 0x7FFF)
		return (_romSpace[0x01][addr - 0x4000]);
	else if (_ramSpace.size() && 0xA000 <= addr && addr <= 0xBFFF)
		return _ramSpace[0x00][addr - 0xA000];
	return (0xFF);
}

uint8_t	cart::getBank(uint16_t addr)
{
	if (addr < 0x4000)
		return 0;
	return 1;
}
#include <iostream>
#include <fstream>
//I anticipate issues with how this is handling rom banks
//TODO: make sure advanced rombanking is properly implemented
mbc1::mbc1(uint32_t romBanks, uint32_t ramBanks, FILE *rom)
{
	printf("MBC1 rom %u ram %u\n", romBanks, ramBanks);
	_romBanks = romBanks;
	_ramBanks = ramBanks;
	_romSpace.resize(romBanks ? romBanks : 2);
	_ramSpace.resize(ramBanks);
	for (uint32_t i = 0x00; i < romBanks; i++)
		fread(&_romSpace[i][0x00], 1, 0x4000, rom);
	_ramg = 0;
	_bank1 = 1;
	_bank2 = 0;
	_mode = 0;
	fclose(rom);
}

void			mbc1::writeTo(uint16_t addr, uint8_t val)
{
//	printf("write 0x%02hhx to 0x%04hx\n", val, addr);
	if (addr <= 0x1FFF)
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

void		mbc1::_ramWrite(uint16_t addr, uint8_t val)
{
	printf("ffffffffffff\n");
	if ((_ramg & 0x0F) != 0x0A)
		return ;
	uint8_t banknum = 0;
	if (_ramSize > 0x2000)
		banknum = _mode ? _bank2 : 0;
//	printf("banknum %u\n", banknum);
	_ramSpace[banknum % _ramBanks][addr] = val;
	if (_ramSize < 0x2000)	//could possibly become a do while
		while (addr < 0x2000)
		{
			addr += _ramSize;
			_ramSpace[banknum % _ramBanks][addr] = val;
		}
}

uint8_t	mbc1::_rombankRead(uint16_t addr)
{
	uint8_t banknum = !_mode ? 0 : _bank2;
	if (addr >= 0x4000)
	{
		banknum = _bank1 | (!_mode ? (_bank2 << 5) : 0);
		addr -= 0x4000;
	}
//	if (banknum)
//		printf("banknum %u\naddr 0x%04x\n", banknum, addr);
	return (_romSpace[banknum % _romBanks][addr]);
}

uint8_t	mbc1::_rambankRead(uint16_t addr)
{
//	printf("we in here?\n");
	uint8_t banknum = !_mode ? 0 : _bank2;
	if ((_ramg & 0x0F) != 0x0A)
		return (0xFF);
//	printf("banknum 0x%02hhx\n", banknum);
	return (_ramSpace[banknum % _ramBanks][addr]);//TODO: verify if ram banks overflow
}

uint8_t	mbc1::readFrom(uint16_t addr)
{
//	printf("we in here 0x%04x\n", addr);
	if (addr <= 0x7FFF)
		return(_rombankRead(addr));
	else if (0xA000 <= addr && addr <= 0xBFFF)
		return (_rambankRead(addr - 0xA000));
	return 0xFF;
}

uint8_t	mbc1::getBank(uint16_t addr)
{
	if (addr < 0x4000)
		return 0;
	else
		return _bank1 | (!_mode ? (_bank2 << 5) : 0);
}

mbc2::mbc2(uint32_t romBanks, uint32_t ramBanks, FILE *rom)
{ //MBC2 chips have a consistent fixed number of ram banks, this variable is only here for ease of mutability
	(void)ramBanks;
	printf("MBC2\n");
	_romSpace.resize(romBanks ? romBanks : 2);
	_ramBanks = ramBanks;
	_ramSize = 0x200;
	for (uint32_t i = 0x00; i < romBanks; i++)
		fread(&_romSpace[i][0x00], 1, 0x4000, rom);
	fclose(rom);
	_ramg = 0;
	_romg = 1;
//	for (uint32_t i = 0x00; i < romBanks; i++)
//		for (uint32_t j = 0x00; j < 0x4000; j++)
//			PRINT_DEBUG("%c", _romSpace[i][j]);
}

void	mbc2::writeTo(uint16_t addr, uint8_t val)
{
	if (addr <= 0x3FFF)
	{
		if ((0x0F00 & addr) == 0x0100)
			_ramg = val;
		else
			_romg = val ? val & 0x0F: 1;
	}
	else if (_ramSize && 0xA000 <= addr && addr <= 0xBFFF)
		_ramWrite(addr - 0xA000, val);
}

uint8_t	mbc2::readFrom(uint16_t addr)
{
	if (addr <= 0x7FFF)
		_rombankRead(addr);
	else if (0xA000 <= addr && addr <= 0xBFFF)
		_ramRead(addr - 0xA000);
	return 0xFF;
}

void	mbc2::_ramWrite(uint16_t addr, uint8_t val)
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

uint8_t	mbc2::_rombankRead(uint16_t addr)
{
	if (addr <= 0x3FFF)
		return (_romSpace[0x00][addr]);
	return (_romSpace[_romg][addr - 0x4000]);
}

uint8_t	mbc2::_ramRead(uint16_t addr)
{
	if ((_ramg & 0x0F) != 0x0A)
		return (0xFF);
	else
		return (_ramSpace[0x00][addr % _ramSize]); //TODO: verify ram behaviour in MBC2
}

uint8_t mbc2::getBank(uint16_t addr)
{
	if (addr < 0x4000)
		return 0;
	return _romg;
}

mbc3::mbc3(uint32_t romBanks, uint32_t ramBanks, FILE *rom)
{
	printf("MBC3\n");
	_romSpace.resize(romBanks ? romBanks : romBanks + 2);
	_ramSpace.resize(ramBanks);
	_ramBanks = ramBanks;
	_ramg = 0;
	_rombank = 1;
	_rambank = 0;
	_timeLatch = 0xFF;
	_rtcS = 0;
	_rtcM = 0;
	_rtcH = 0;
	_rtcDL = 0;
	_rtcDH = 0;
	for (uint32_t i = 0x00; i < romBanks; i++)
		fread(&_romSpace[i][0x00], 1, 0x4000, rom);
	fclose(rom);
}

void			mbc3::writeTo(uint16_t addr, uint8_t val)
{
	if (addr <= 0x1FFF)
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

void		mbc3::_latchTime(uint8_t val)
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

void		mbc3::_ramWrite(uint16_t addr, uint8_t val) //TODO: verify overflow behaviour of MBC2 and MBC3
{
	if (_ramg != 0x0A)
		return ;
	uint8_t banknum = 0;
	if (_ramSize > 0x2000)
		banknum = _rambank;
	_ramSpace[banknum % _ramBanks][addr] = val;
	if (_ramSize < 0x2000)
	{
		addr += _ramSize;
		while (addr < 0x2000)
		{
			_ramSpace[banknum % _ramBanks][addr] = val;
			addr += _ramSize;
		}
	}
}

uint8_t	mbc3::_rombankRead(uint16_t addr)
{
	uint8_t banknum = 0;
	if (addr >= 0x4000)
	{
		banknum = _rombank;
		addr -= 0x4000;
	}
	return (_romSpace[banknum % _romBanks][addr]);
}

uint8_t	mbc3::_rambankRead(uint16_t addr)
{
	if ((_ramg & 0x0F) != 0x0A)
		return (0xFF);
	if (_rambank < 0x08)
		return (_ramSpace[_rambank % _ramBanks][addr]);
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

uint8_t	mbc3::readFrom(uint16_t addr)
{
	if (addr <= 0x7FFF)
		return(_rombankRead(addr));
	else if (0xA000 <= addr && addr <= 0xBFFF)
		return (_rambankRead(addr - 0xA000));
	return 0xFF;
}

uint8_t	mbc3::getBank(uint16_t addr)
{
	(void)addr; //verify that I am properly accessing banks in different places
				//and that this is only here for easy of mutability
	return _rombank;
}

mbc5::mbc5(uint32_t romBanks, uint32_t ramBanks, FILE *rom)
{
	printf("MBC5\n");
	_romSpace.resize(romBanks ? romBanks : 2);
	_ramSpace.resize(ramBanks);
	_ramBanks = ramBanks;
	for (uint32_t i = 0x00; i < romBanks; i++)
		fread(&_romSpace[i][0x00], 1, 0x4000, rom);
	_ramg = 0;
	_bank1 = 0;
	_bank2 = 0;
	_rambank = 0;
	fclose(rom);
}

#include <stdlib.h>
void			mbc5::writeTo(uint16_t addr, uint8_t val)
{
	if (addr <= 0x1FFF)
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

void		mbc5::_ramWrite(uint16_t addr, uint8_t val)
{
	if ((_ramg & 0x0F) != 0x0A)
		return ;
	uint8_t banknum = 0;
	if (_ramSize > 0x2000)
		banknum = _rambank;
	printf("ram bank %u\n", _rambank);
	_ramSpace[banknum % _ramBanks][addr] = val;
	if (_ramSize < 0x2000)
	{
		addr += _ramSize;
		while (addr < 0x2000)
		{
			_ramSpace[banknum % _ramBanks][addr] = val;
			addr += _ramSize;
		}
	}
}

uint8_t	mbc5::_rombankRead(uint16_t addr)
{
	uint16_t banknum = 0;
	if (addr >= 0x4000)
	{
		banknum = _bank1 | (_bank2 ? 0x0100 : 0x0000);
		addr -= 0x4000;
	}
	return (_romSpace[banknum % _romBanks][addr]);
}

uint8_t	mbc5::_rambankRead(uint16_t addr)
{
	uint8_t banknum = 0;
//	if ((_ramg & 0x0F) != 0x0A)
	if (_ramg != 0x0A)
		return (0xFF);
	if (_ramSize > 0x2000)
		banknum = _rambank;
	return (_ramSpace[banknum % _ramBanks][addr]);
}

uint8_t	mbc5::readFrom(uint16_t addr)
{
	if (addr <= 0x7FFF)
		return(_rombankRead(addr));
	else if (0xA000 <= addr && addr <= 0xBFFF)
		return (_rambankRead(addr - 0xA000));
	return 0xFF;
}

uint8_t	mbc5::getBank(uint16_t addr)
{
	(void)addr; //similar to mbc3
	return _bank1 | (_bank2 ? 0x0100 : 0x0000);
}