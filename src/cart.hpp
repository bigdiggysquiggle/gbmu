#ifndef CART
#define CART
#include <vector>
#include <array>
#include <memory>
#include <cstdio>

// eventually implement ram loading from actual save files

//				cart type
//
//00h  ROM ONLY                 13h  MBC3+RAM+BATTERY
//01h  MBC1                     15h  MBC4
//02h  MBC1+RAM                 16h  MBC4+RAM
//03h  MBC1+RAM+BATTERY         17h  MBC4+RAM+BATTERY
//05h  MBC2                     19h  MBC5
//06h  MBC2+BATTERY             1Ah  MBC5+RAM
//08h  ROM+RAM                  1Bh  MBC5+RAM+BATTERY
//09h  ROM+RAM+BATTERY          1Ch  MBC5+RUMBLE
//0Bh  MMM01                    1Dh  MBC5+RUMBLE+RAM
//0Ch  MMM01+RAM                1Eh  MBC5+RUMBLE+RAM+BATTERY
//0Dh  MMM01+RAM+BATTERY        FCh  POCKET CAMERA
//0Fh  MBC3+TIMER+BATTERY       FDh  BANDAI TAMA5
//10h  MBC3+TIMER+RAM+BATTERY   FEh  HuC3
//11h  MBC3                     FFh  HuC1+RAM+BATTERY
//12h  MBC3+RAM
//
//				rom size
//
//00h -  32KByte (no ROM banking)
//01h -  64KByte (4 banks)
//02h - 128KByte (8 banks)
//03h - 256KByte (16 banks)
//04h - 512KByte (32 banks)
//05h -   1MByte (64 banks)  - only 63 banks used by MBC1
//06h -   2MByte (128 banks) - only 125 banks used by MBC1
//07h -   4MByte (256 banks)
//52h - 1.1MByte (72 banks)
//53h - 1.2MByte (80 banks)
//54h - 1.5MByte (96 banks)
//
//				ram size
//
//00h - None
//01h - 2 KBytes
//02h - 8 Kbytes
//03h - 32 KBytes (4 banks of 8KBytes each)
//
//have mbcs with batteries handle file saving
//
//verify if mbc1 carts only ever have ram


class cart {
	public:
		cart();
		cart(FILE *, unsigned short);
		std::unique_ptr<cart>	loadCart(char *fname);
		virtual void writeTo(unsigned short, unsigned char);
		virtual unsigned char readFrom(unsigned short);
		unsigned char	GBbit;
		virtual unsigned char	getBank(unsigned short);
	protected:
//		std::vector<unsigned char[0x4000]> _romSpace;
//		std::vector<unsigned char[0x2000]> _ramSpace;
		std::vector<std::array<unsigned char, 0x4000>>	_romSpace;
		std::vector<std::array<unsigned char, 0x2000>>	_ramSpace;
		unsigned	_romSize;
		unsigned	_ramSize;
		unsigned short	romSizetab(unsigned char);
		unsigned short	ramSizetab(unsigned char);
};


// all binary notes only include the used bits.
// other bits are ignored
// ram gate:
//	0x0000-0x1FFF - 0b1010 when writable
//	0x2000-0x3FFF - denotes lower bits of bank value
// bank1:
//	used when cpu accesses 0x4000-0x7FFFF in the memory
//	area. 0b00001 is the minimum value
// bank2:
//	0x4000-0x5FFF - can be either upper bits of rom
//	bank value or the ram bank value. can be 0
// mode:
//	0x6000-0x7FFF - can only be 1 or 0. determines what
//	bank2 does. 0 usually makes it control rom, 1 ram.
//	exceptions are multicart games and 8mbit or 16mbit
//	rom chips.
//	- when 0 bank2 affects 0x4000-0x7FFF
//	- when 1 0x0000-0x3FFF, 0x4000-0x7FFF, 0xA000-0xBFFF

//implement multicart later
//
//Most documentation, including Pan Docs [3], calls value
//0b0 ROM banking mode, and value 0b1 RAM banking mode. This
//terminology reflects the common use cases, but "RAM banking"
//is slightly mis-leading because value 0b1 also affects ROM
//reads in multicart cartridges and cartridges that have a 8
//or 16 Mbit ROM chip.

class mbc1 : virtual public cart{
	//max 2MB ROM and/or 32KB RAM
	public:
		mbc1(unsigned romBanks, unsigned ramBanks, FILE *rom);
		void	writeTo(unsigned short, unsigned char);
		unsigned char	readFrom(unsigned short);
		unsigned char	getBank(unsigned short);
	private:
		unsigned char	_ramg;
		unsigned char	_bank1;
		unsigned char	_bank2;
		unsigned char	_mode;

		void	_ramWrite(unsigned short addr, unsigned char val);
		unsigned char _rombankRead(unsigned short);
		unsigned char _rambankRead(unsigned short);
};

//RAMG and ROMB both accessible by write to 0x0000-0x3FFF
//depends on signal from A8 address signal
//-in practice, addr & 0x100
//if 0 - RAMG, 1 - ROMG

class mbc2 : virtual public cart{
	//max 256KB ROM and 512x4 bits RAM
	public:
		mbc2(unsigned romBanks, unsigned ramBanks, FILE *rom);
		void	writeTo(unsigned short, unsigned char);
		unsigned char	readFrom(unsigned short);
		unsigned char	getBank(unsigned short);
	private:
		unsigned char	_ramg;
		unsigned char	_romg;

		void	_ramWrite(unsigned short addr, unsigned char val);
		unsigned char _rombankRead(unsigned short);
		unsigned char _ramRead(unsigned short);
};

class mbc3 : virtual public cart{
	//max 2MB ROM and/or 32KB RAM
	public:
		mbc3(unsigned romBanks, unsigned ramBanks, FILE *rom);
		void	writeTo(unsigned short, unsigned char);
		unsigned char	readFrom(unsigned short);
		unsigned char	getBank(unsigned short);
	private:
		unsigned char	_ramg;
		unsigned char	_rombank;
		unsigned char	_rambank;
		unsigned char	_timeLatch;
		unsigned char	_rtcS;
		unsigned char	_rtcM;
		unsigned char	_rtcH;
		unsigned char	_rtcDL;
		unsigned char	_rtcDH;
		//write a function to set rtc registers on startup
		//
		//The total 9 bits of the Day Counter allow to count
		//days in range from 0-511 (0-1FFh). The Day Counter
		//Carry Bit becomes set when this value overflows. In
		//that case the Carry Bit remains set until the
		//program does reset it. Note that you can store an of
		//set to the Day Counter in battery RAM. For example,
		//every time you read a non-zero Day Counter, add this
		//Counter to the offset in RAM, and reset the Counter
		//to zero. This method allows to count any number of
		//days, making your program Year-10000-Proof, provided
		//that the cartridge gets used at least every 511 days

		void	_ramWrite(unsigned short addr, unsigned char val);
		unsigned char _rombankRead(unsigned short);
		unsigned char _rambankRead(unsigned short);
		void	_latchTime(unsigned char);
};

class mbc5 : virtual public cart{
	//max 8MByte ROM and/or 128KByte RAM)
	public:
		mbc5(unsigned romBanks, unsigned ramBanks, FILE *rom);
		void	writeTo(unsigned short, unsigned char);
		unsigned char	readFrom(unsigned short);
		unsigned char	getBank(unsigned short);
	private:
		unsigned char	_ramg;
		unsigned char	_bank1;
		unsigned char	_bank2;
		unsigned char	_rambank;

		void	_ramWrite(unsigned short addr, unsigned char val);
		unsigned char _rombankRead(unsigned short);
		unsigned char _rambankRead(unsigned short);
};

#endif
