#ifndef CART
#define CART
#include <vector>
#include <array>
#include <memory>
#include <cstdio>
#include <cstdint>

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
		cart(FILE *, uint16_t);
		std::unique_ptr<cart>	loadCart(char *fname);
		virtual void writeTo(uint16_t, uint8_t);
		virtual uint8_t readFrom(uint16_t);
		uint8_t	GBbit;
		virtual uint8_t	getBank(uint16_t);
	protected:
//		std::vector<uint8_t[0x4000]> _romSpace;
//		std::vector<uint8_t[0x2000]> _ramSpace;
		std::vector<std::array<uint8_t, 0x4000>>	_romSpace;
		std::vector<std::array<uint8_t, 0x2000>>	_ramSpace;
		uint32_t	_romSize;
		uint32_t	_ramSize;
		uint16_t	romSizetab(uint8_t);
		uint16_t	ramSizetab(uint8_t);
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
		mbc1(uint32_t romBanks, uint32_t ramBanks, FILE *rom);
		void	writeTo(uint16_t, uint8_t);
		uint8_t	readFrom(uint16_t);
		uint8_t	getBank(uint16_t);
	private:
		uint32_t		_romBanks;
		uint8_t	_ramg;
		uint8_t	_bank1;
		uint8_t	_bank2;
		uint8_t	_mode;

		void	_ramWrite(uint16_t addr, uint8_t val);
		uint8_t _rombankRead(uint16_t);
		uint8_t _rambankRead(uint16_t);
};

//RAMG and ROMB both accessible by write to 0x0000-0x3FFF
//depends on signal from A8 address signal
//-in practice, addr & 0x100
//if 0 - RAMG, 1 - ROMG

class mbc2 : virtual public cart{
	//max 256KB ROM and 512x4 bits RAM
	public:
		mbc2(uint32_t romBanks, uint32_t ramBanks, FILE *rom);
		void	writeTo(uint16_t, uint8_t);
		uint8_t	readFrom(uint16_t);
		uint8_t	getBank(uint16_t);
	private:
		uint8_t	_ramg;
		uint8_t	_romg;

		void	_ramWrite(uint16_t addr, uint8_t val);
		uint8_t _rombankRead(uint16_t);
		uint8_t _ramRead(uint16_t);
};

class mbc3 : virtual public cart{
	//max 2MB ROM and/or 32KB RAM
	public:
		mbc3(uint32_t romBanks, uint32_t ramBanks, FILE *rom);
		void	writeTo(uint16_t, uint8_t);
		uint8_t	readFrom(uint16_t);
		uint8_t	getBank(uint16_t);
	private:
		uint8_t	_ramg;
		uint8_t	_rombank;
		uint8_t	_rambank;
		uint8_t	_timeLatch;
		uint8_t	_rtcS;
		uint8_t	_rtcM;
		uint8_t	_rtcH;
		uint8_t	_rtcDL;
		uint8_t	_rtcDH;
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

		void	_ramWrite(uint16_t addr, uint8_t val);
		uint8_t _rombankRead(uint16_t);
		uint8_t _rambankRead(uint16_t);
		void	_latchTime(uint8_t);
};

class mbc5 : virtual public cart{
	//max 8MByte ROM and/or 128KByte RAM)
	public:
		mbc5(uint32_t romBanks, uint32_t ramBanks, FILE *rom);
		void	writeTo(uint16_t, uint8_t);
		uint8_t	readFrom(uint16_t);
		uint8_t	getBank(uint16_t);
	private:
		uint8_t	_ramg;
		uint8_t	_bank1;
		uint8_t	_bank2;
		uint8_t	_rambank;

		void	_ramWrite(uint16_t addr, uint8_t val);
		uint8_t _rombankRead(uint16_t);
		uint8_t _rambankRead(uint16_t);
};

#endif
