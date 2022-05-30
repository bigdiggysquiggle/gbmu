#include "print_debug.hpp"
#include "cpu.hpp"
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#define X(x) ((x) >> 6)
#define Y(x) (((x) >> 3) & 7)
#define Z(x) ((x) & 7)
#define P(x) (((x) >> 4) & 3)
#define Q(x) (((x) >> 3) & 1)
#define FNZ	(_registers.f & bitflags::z ? 0 : 1)
#define FZ	(_registers.f & bitflags::z ? 1 : 0)
#define FNC (_registers.f & bitflags::cy ? 0 : 1)
#define FC (_registers.f & bitflags::cy ? 1 : 0)

//Disregard the mess. The vast majority of these are functions used to emulate
//CPU intructions. Notably, I have inserted pauses into each instruction for
//every 4 cycles so that the PPU can handle graphics in parallel with the CPU
//as happens on real hardware

void	debug_print(unsigned char op, unsigned char cb, struct registers reg, unsigned char lcdc, unsigned char stat, unsigned char ly, unsigned char lyc, unsigned char IF, unsigned char IE, unsigned char IME);

cpu::cpu(std::shared_ptr<mmu> unit, std::shared_ptr<ppu> pp) : _mmu(unit), _ppu(pp)
{
	//have gb run cart verification before this
//	_registers.af = 0x01B0; //0x11B0 for cgb and 0xffB0 for pocket
//	_registers.bc = 0x13;
//	_registers.de = 0xD8;
//	_registers.hl = 0x14D;
//	_registers.pc = 0x100;
//	_registers.sp = 0xFFFE;
	_registers.af = 0x00;
	_registers.bc = 0x00;
	_registers.de = 0x00;
	_registers.hl = 0x00;
	_registers.pc = 0x00;
	_registers.sp = 0x00;
	_halt = false;
	haltcheck = 1;
}

int	cpu::imeCheck()
{
	if (ime_set)
	{
		_ime = (ime_set % 2);
		ime_set = 0;
		return 1;
	}
	return 0;
}

unsigned char	nLogo[]={
	0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03,
	0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D, 0x00, 0x08,
	0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E,
	0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63,
	0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB,
	0xB9, 0x33, 0x3E};

/*void	cpu::checkRom(void)
{
	unsigned short start = 0x104;
	unsigned short end = _registers.a == 0x01 ? 0x133 : 0x11C;
	while (start <= end && nLogo[start - 0x104] == _mmu->accessAt(start))
		start++;
	if (start <= end)
		throw "Error: invalid cart";
	unsigned char modecheck = _mmu->accessAt(0x143);
	if (_registers.a == 0x11 && (modecheck == 0x80 || modecheck == 0xC0))
		_mmu->_cgb_mode = 0x03;

}*/

unsigned char	cpu::interrupt_check(void)
{
	int i = 0;
	if (_halt == true || _ime)
	{
//		PRINT_DEBUG("interrupt checking");
		unsigned short	targets[] = {
			0x40, 0x48, 0x50, 0x58, 0x60};
		unsigned char 	intif = _mmu->accessAt(0xFF0F);
		int c;
		for (c = 1; c <= 0x1F && !(c & intif); c <<= 1)
			i++;
		if (c <= 0x1F && (c & _mmu->accessAt(0xFFFF)))
		{
//			PRINT_DEBUG("jump to 0x%02x", targets[i]);
			if (_ime)
			{
				_mmu->writeTo(0xFF0F, intif & ~c);
				_ime = 0;
				call(targets[i]);
			}
			if (_halt == true)
			{
				_halt = false;
				haltcheck = _ime ? 1 : 0;
//			   return opcode_parse(_ime ? 1 : 0);	
			}
		}
	}
	return (0);
}

// maybe write wrapper function to access mmu based on mode

void	cpu::reset()
{
	_registers.pc = 0;
	_mmu->writeTo(0xFF40, 0x00);
}

void	cpu::setInterrupt(unsigned char INT)
{
	//1 VBlank 2 STAT 4 Timer 8 Serial 16 Joypad
	//VBlank happens beginning of every VBlank
	//STAT triggers on conditions set in STAT reg
	//Timer triggers on a frequency determined by a set of registers
	////when 0xFF05 goes from FF to 00
	//Serial triggers when a byte is received via serial port
	//Joypad triggers on button press
	INT |= _mmu->accessAt(0xFF0F);
	_mmu->writeTo(0xFF0F, INT);
}

void	cpu::ld(unsigned char *reg, unsigned char val)
{
	if (reg)
		*reg = val;
	else
	{
		cycle();
		_mmu->writeTo(_registers.hl, val);
	}
}

//below might need to handle (C + 0xFF00) and (n + 0xFF00)

void	cpu::ld(unsigned char *regd, unsigned short addr)
{
	if (regd)
	{
		cycle();
		*regd = _mmu->accessAt(addr);
	}
	else
	{
		cycle();
		cyc += 4;
		_mmu->writeTo(_registers.hl, _mmu->accessAt(addr));
	}
}

void	cpu::ldd(unsigned char a)
{
	if (a)
		_registers.a = _mmu->accessAt(_registers.hl);
	else
		_mmu->writeTo(_registers.hl, _registers.a);
	cycle();
	_registers.hl = _registers.hl - 1;
}

void	cpu::ldi(unsigned char a)
{
	if (a)
		_registers.a = _mmu->accessAt(_registers.hl);
	else
		_mmu->writeTo(_registers.hl, _registers.a);
	cycle();
	_registers.hl++;
}

void	cpu::ld(unsigned short *regp, unsigned short val)
{
	*regp = val;
}

void	cpu::ld(union address addr, unsigned char val)
{
	cycle();
	_mmu->writeTo(addr.addr, val);
}

void	cpu::ld(union address addr, union address val)
{
	cycle();
	_mmu->writeTo(addr.addr, val.n1);
	_mmu->writeTo(addr.addr + 1, val.n2);
}

void	cpu::ldhl(char n)
{
	unsigned short dis;
	unsigned char uns = n;
	unsigned res;
	if (((_registers.sp & 0x0F) + (uns & 0x0F)) & 0xF0)
		_registers.f |= bitflags::h;
	else
		_registers.f &= ~(bitflags::h);
	if (((_registers.sp & 0xFF) + (uns & 0xFF)) & 0xF00)
		_registers.f |= bitflags::cy;
	else
		_registers.f &= ~(bitflags::cy);
	cycle();
	if (n < 0)
	{
		dis = -n;
		res = _registers.sp - dis;
	}
	else
	{
		dis = n;
		res = _registers.sp + dis;
	}
	_registers.f &= ~(bitflags::z | bitflags::n);
	cycle();
	_registers.hl = res;
}

void	cpu::push(unsigned short *regp)
{
	union address val;
	val.addr = *regp;
	cycle();
	_mmu->writeTo(--_registers.sp, val.n2);
	cycle();
	_mmu->writeTo(--_registers.sp, val.n1);
	cycle();
}

void	cpu::pop(unsigned short *regp)
{
	union address val;//endian is gonna kill me I swear
	cycle();
	val.n1 = _mmu->accessAt(_registers.sp++);
	cycle();
	val.n2 = _mmu->accessAt(_registers.sp++);
	*regp = val.addr;
}

void	cpu::add(unsigned short *regp)
{
	cycle();
	unsigned res = *regp + _registers.hl;
	_registers.f &= ~(bitflags::n);
	if (((_registers.hl & 0x0FFF) + (*regp & 0x0FFF)) & 0xF000)
		_registers.f |= bitflags::h;
	else
		_registers.f &= ~(bitflags::h);
	if (res & 0xF0000)
		_registers.f |= bitflags::cy;
	else
		_registers.f &= ~(bitflags::cy);
	_registers.hl = res;;
}

void	cpu::add(char val)
{
	unsigned short dis;
	unsigned char uns = val;
	unsigned res;
	if (((_registers.sp & 0x0F) + (uns & 0x0F)) & 0xF0)
		_registers.f |= bitflags::h;
	else
		_registers.f &= ~(bitflags::h);
	if (((_registers.sp & 0xFF) + (uns & 0xFF)) & 0xF00)
		_registers.f |= bitflags::cy;
	else
		_registers.f &= ~(bitflags::cy);
	if (val < 0)
	{
		dis = -val;
		res = _registers.sp - dis;
	}
	else
	{
		dis = val;
		res = _registers.sp + dis;
	}
	_registers.f &= ~(bitflags::z | bitflags::n);
	_registers.sp = res;
	cycle();
	cycle();
}

void	cpu::add(unsigned char val)
{
	unsigned short res = _registers.a + val;
	if ((((_registers.a & 0x0F) + (val & 0x0F)) & 0x10) == 0x10)
		_registers.f |= bitflags::h;
	else
		_registers.f &= ~(bitflags::h);
	if (res & 0xF00)
		_registers.f |= bitflags::cy;
	else
		_registers.f &= ~(bitflags::cy);
	_registers.a = res;
	if (!_registers.a)
		_registers.f |= bitflags::z;
	else
		_registers.f &= ~(bitflags::z);
	_registers.f &= ~(bitflags::n);
}

void	cpu::adc(unsigned char val)
{
	unsigned char c = (_registers.f & bitflags::cy) ? 1 : 0;
	unsigned short res = _registers.a + val + c;
	if ((((_registers.a & 0x0F) + (val & 0x0F) + c) & 0x10) == 0x10)
		_registers.f |= bitflags::h;
	else
		_registers.f &= ~(bitflags::h);
	if (res & 0xF00)
		_registers.f |= bitflags::cy;
	else
		_registers.f &= ~(bitflags::cy);
	_registers.a = res;
	if (!_registers.a)
		_registers.f |= bitflags::z;
	else
		_registers.f &= ~(bitflags::z);
	_registers.f &= ~(bitflags::n);
}

void	cpu::sub(unsigned char val)
{
	unsigned short res = _registers.a - val;
	if ((((_registers.a & 0x0F) - (val & 0x0F)) & 0x10) == 0x10)
		_registers.f |= bitflags::h;
	else
		_registers.f &= ~(bitflags::h);
	if (res & 0xF00)
		_registers.f |= bitflags::cy;
	else
		_registers.f &= ~(bitflags::cy);
	_registers.a = res;
	if (!_registers.a)
		_registers.f |= bitflags::z;
	else
		_registers.f &= ~(bitflags::z);
	_registers.f |= bitflags::n;
}

void	cpu::sbc(unsigned char val)
{
	unsigned char c = (_registers.f & bitflags::cy) ? 1 : 0;
	unsigned short res = _registers.a - (val + c);
	if ((((_registers.a & 0x0F) - ((val & 0x0F) + c)) & 0x10) == 0x10)
		_registers.f |= bitflags::h;
	else
		_registers.f &= ~(bitflags::h);
	if (res & 0xF00)
		_registers.f |= bitflags::cy;
	else
		_registers.f &= ~(bitflags::cy);
	_registers.a = res;
	if (!_registers.a)
		_registers.f |= bitflags::z;
	else
		_registers.f &= ~(bitflags::z);
	_registers.f |= bitflags::n;
}

void	cpu::_or(unsigned char val)
{
	_registers.a |= val;
	_registers.f &= ~(bitflags::n + bitflags::cy + bitflags::h);
	if (!_registers.a)
		_registers.f |= bitflags::z;
	else
		_registers.f &= ~(bitflags::z);
}

void	cpu::_xor(unsigned char val)
{
	_registers.a ^= val;
	_registers.f &= ~(bitflags::n + bitflags::cy + bitflags::h);
	if (!_registers.a)
		_registers.f |= bitflags::z;
	else
		_registers.f &= ~(bitflags::z);
}

void	cpu::_and(unsigned char val)
{
	_registers.a &= val;
	_registers.f &= ~(bitflags::n | bitflags::cy);
	_registers.f |= bitflags::h;
	if (!_registers.a)
		_registers.f |= bitflags::z;
	else
		_registers.f &= ~(bitflags::z);
}

void	cpu::cp(unsigned char val)
{
	unsigned short res;

	if ((((_registers.a & 0x0F) - (val & 0x0F)) & 0x10) == 0x10)
		_registers.f |= bitflags::h;
	else
		_registers.f &= ~(bitflags::h);
	res = _registers.a - val;
	if (res & 0xF00)
		_registers.f |= bitflags::cy;
	else
		_registers.f &= ~(bitflags::cy);
	if (!(_registers.a - val))
		_registers.f |= bitflags::z;
	else
		_registers.f &= ~(bitflags::z);
	_registers.f |= bitflags::n;
}

void	cpu::inc(unsigned char *reg)
{
	unsigned char val;
	if (reg)
		val = *reg;
	else
	{
		cycle();
		val = _mmu->accessAt(_registers.hl);
		cycle();
	}
	if ((((val & 0x0F) + 1) & 0x10) == 0x10)
		_registers.f |= bitflags::h;
	else
		_registers.f &= ~(bitflags::h);
	val++;
	if (!val)
		_registers.f |= bitflags::z;
	else
		_registers.f &= ~(bitflags::z);
	_registers.f &= ~(bitflags::n);
	if (reg)
		*reg = val;
	else
		_mmu->writeTo(_registers.hl, val);
}

void	cpu::inc(unsigned short *regp)
{
	cycle();
	*regp += 1;
}

void	cpu::dec(unsigned char *reg)
{
	unsigned char val;
	if (reg)
		val = *reg;
	else
	{
		cycle();
		val = _mmu->accessAt(_registers.hl);
		cycle();
	}
	if ((((val & 0x0F) - 1) & 0x10) == 0x10)
		_registers.f |= bitflags::h;
	else
		_registers.f &= ~(bitflags::h);
	val--;
	if (!val)
		_registers.f |= bitflags::z;
	else
		_registers.f &= ~(bitflags::z);
	_registers.f |= bitflags::n;
	if (reg)
		*reg = val;
	else
		_mmu->writeTo(_registers.hl, val);
}

void	cpu::dec(unsigned short *regp)
{
	cycle();
	*regp = *regp - 1;
}

void	cpu::swap(unsigned char *reg)
{
	cycle();
	unsigned char val;
	val = reg ? *reg : _mmu->accessAt(_registers.hl);
	val = val >> 4 | val << 4;
	if (reg)
		*reg = val;
	else
	{
		cycle();
		_mmu->writeTo(_registers.hl, val);
		cycle();
	}
	_registers.f &= ~(bitflags::z + bitflags::n + bitflags::h + bitflags::cy);
	if (!val)
		_registers.f |= bitflags::z;
}


void	cpu::daa(void)
{
// note: assumes a is a uint8_t and wraps from 0xff to 0
	if (!(_registers.f & bitflags::n))
	{  // after an addition, adjust if (half-)carry occurred or if result is out of bounds
		if ((_registers.f & bitflags::cy) || _registers.a > 0x99)
		{
			_registers.a += 0x60;
			_registers.f |= bitflags::cy;
		}
		if ((_registers.f & bitflags::h) || (_registers.a & 0x0F) > 0x09)
			_registers.a += 0x06;
	}
	else
	{  // after a subtraction, only adjust if (half-)carry occurred
		if (_registers.f & bitflags::cy)
			_registers.a -= 0x60;
		if (_registers.f & bitflags::h)
			_registers.a -= 0x6;
	}
// these flags are always updated
	_registers.f = !_registers.a ? _registers.f | bitflags::z : _registers.f & ~(bitflags::z);
	_registers.f &= ~(bitflags::h);
}

void	cpu::cpl(void)
{
	_registers.a = ~(_registers.a);
	_registers.f |= (bitflags::n | bitflags::h);
}

void	cpu::ccf(void)
{
	_registers.f ^= bitflags::cy;
	_registers.f &= ~(bitflags::n | bitflags::h);
}

void	cpu::scf(void)
{
	_registers.f |= bitflags::cy;
	_registers.f &= ~(bitflags::n | bitflags::h);
}

void	cpu::halt(void)//halt waits for an IF & IE != 0
{
	_halt = true;
}

//just realized my stop probably infinite loops. my b
//reimplement in a manner similar to halt.
void	cpu::stop(void)//check p1 bits and p1 line
{
	while ((_mmu->accessAt(0xFF00) & 0xFF) == 0xFF)
		continue;
}
/*
unsigned char	cpu::di(void)
{
	cyc += opcode_parse();
	this->_ime = 0;
	return cyc;
}

unsigned char	cpu::ei(void)
{
	cyc += opcode_parse();
	this->_ime = 1;
	return cyc;
}
*/

unsigned char	cpu::di()
{
	ime_set = 2;
	return 0;
}

unsigned char	cpu::ei()
{
	ime_set = 1;
	return 0;
}

void	cpu::rlca(void)
{
	_registers.a = (_registers.a >> 7) | (_registers.a << 1);
	_registers.f = (_registers.a & 1) ? (_registers.f | bitflags::cy) : (_registers.f & ~(bitflags::cy));
	_registers.f &= ~(bitflags::n + bitflags::h + bitflags::z);
}

void	cpu::rla(void)
{
	unsigned char cb = (_registers.f >> 4) & 0x01;
	_registers.f = (_registers.a & 0x80) ? (_registers.f | bitflags::cy) : (_registers.f & ~(bitflags::cy));
	_registers.a = cb | (_registers.a << 1);
	_registers.f &= ~(bitflags::n + bitflags::h + bitflags::z);
}

void	cpu::rrca(void)
{
	_registers.a = (_registers.a << 7) | (_registers.a >> 1);
	_registers.f = (_registers.a & 0x80) ? (_registers.f | bitflags::cy) : (_registers.f & ~(bitflags::cy));
	_registers.f &= ~(bitflags::n + bitflags::h + bitflags::z);
}

void	cpu::rra(void)
{
	unsigned char cb = (_registers.f >> 4) & 0x01;
	_registers.f = (_registers.a & 0x01) ? (_registers.f | bitflags::cy) : (_registers.f & ~(bitflags::cy));
	_registers.a = (cb << 7) | (_registers.a >> 1);
	_registers.f &= ~(bitflags::n + bitflags::h + bitflags::z);
}

void	cpu::rlc(unsigned char *reg)
{
	cycle();
	unsigned char val;
	val = reg ? *reg : _mmu->accessAt(_registers.hl);
	val = (val << 1) | (val >> 7);
	_registers.f = (val & 0x01) ? (_registers.f | bitflags::cy) : (_registers.f & ~(bitflags::cy));
	if (!val)
		_registers.f |= bitflags::z;
	else
		_registers.f &= ~(bitflags::z);
	_registers.f &= ~(bitflags::n | bitflags::h);
	if (reg)
		*reg = val;
	else
	{
		cycle();
		_mmu->writeTo(_registers.hl, val);
		cycle();
	}
}

void	cpu::rl(unsigned char *reg)
{
	cycle();
	unsigned char cb = (_registers.f >> 4) & 0x01;
	unsigned char val;
	val = reg ? *reg : _mmu->accessAt(_registers.hl);
	_registers.f = (val & 0x80) ? (_registers.f | bitflags::cy) : (_registers.f & ~(bitflags::cy));
	val = (val << 1) | cb;
	if (!val)
		_registers.f |= bitflags::z;
	else
		_registers.f &= ~(bitflags::z);
	_registers.f &= ~(bitflags::n | bitflags::h);
	if (reg)
		*reg = val;
	else
	{
		cycle();
		_mmu->writeTo(_registers.hl, val);
		cycle();
	}
}

void	cpu::rrc(unsigned char *reg)
{
	cycle();
	unsigned char val;
	val = reg ? *reg : _mmu->accessAt(_registers.hl);
	_registers.f = (val & 0x01) ? (_registers.f | bitflags::cy) : (_registers.f & ~(bitflags::cy));
	val = (val >> 1) | (val << 7);
	if (!val)
		_registers.f |= bitflags::z;
	else
		_registers.f &= ~(bitflags::z);
	_registers.f &= ~(bitflags::n | bitflags::h);
	if (reg)
		*reg = val;
	else
	{
		cycle();
		_mmu->writeTo(_registers.hl, val);
		cycle();
	}
}

void	cpu::rr(unsigned char *reg)
{
	cycle();
	unsigned char cb = (_registers.f >> 4) & 0x01;
	unsigned char val;
	val = reg ? *reg : _mmu->accessAt(_registers.hl);
	_registers.f = (val & 0x01) ? (_registers.f | bitflags::cy) : (_registers.f & ~(bitflags::cy));
	val = (val >> 1) | (cb << 7);
	if (!val)
		_registers.f |= bitflags::z;
	else
		_registers.f &= ~(bitflags::z);
	_registers.f &= ~(bitflags::n | bitflags::h);
	if (reg)
		*reg = val;
	else
	{
		cycle();
		_mmu->writeTo(_registers.hl, val);
		cycle();
	}
}

void	cpu::sla(unsigned char *reg)
{
	cycle();
	unsigned char val;
	val = reg ? *reg : _mmu->accessAt(_registers.hl);
	_registers.f = (val & 0x80) ? (_registers.f | bitflags::cy) : (_registers.f & ~(bitflags::cy));
	val <<= 1;
	if (!val)
		_registers.f |= bitflags::z;
	else
		_registers.f &= ~(bitflags::z);
	_registers.f &= ~(bitflags::n | bitflags::h);
	if (reg)
		*reg = val;
	else
	{
		cycle();
		_mmu->writeTo(_registers.hl, val);
		cycle();
	}
}

void	cpu::sra(unsigned char *reg)
{
	cycle();
	unsigned char val;
	val = reg ? *reg : _mmu->accessAt(_registers.hl);
	unsigned char bit = (val & 0x80);
	_registers.f = (val & 0x01) ? (_registers.f | bitflags::cy) : (_registers.f & ~(bitflags::cy));
	val >>= 1;
	val |= bit;
	if (!val)
		_registers.f |= bitflags::z;
	else
		_registers.f &= ~(bitflags::z);
	_registers.f &= ~(bitflags::n | bitflags::h);
	if (reg)
		*reg = val;
	else
	{
		cycle();
		_mmu->writeTo(_registers.hl, val);
		cycle();
	}
}

void	cpu::srl(unsigned char *reg)
{
	cycle();
	unsigned char val;
	val = reg ? *reg : _mmu->accessAt(_registers.hl);
	_registers.f = (val & 0x01) ? (_registers.f | bitflags::cy) : (_registers.f & ~(bitflags::cy));
	_registers.f &= ~(bitflags::h | bitflags::n);
	val >>= 1;
	if (!val)
		_registers.f |= bitflags::z;
	else
		_registers.f &= ~(bitflags::z);
	if (reg)
		*reg = val;
	else
	{
		cycle();
		_mmu->writeTo(_registers.hl, val);
		cycle();
	}
}

void	cpu::bit(unsigned char opcode)
{
	cycle();
	unsigned char *_regtab[] = {
		&_registers.b,
		&_registers.c,
		&_registers.d,
		&_registers.e,
		&_registers.h,
		&_registers.l,
		0,
		&_registers.a };
	unsigned char bit = 1 << Y(opcode);
	unsigned char *reg = _regtab[Z(opcode)];
	unsigned char val = reg ? *reg : _mmu->accessAt(_registers.hl);
	if (val & bit)
		_registers.f &= ~(bitflags::z);
	else
		_registers.f |= bitflags::z;
	_registers.f |= bitflags::h;
	_registers.f &= ~(bitflags::n);
	if (!reg)
	{
		cycle();
		cycle();
	}
}

void	cpu::set(unsigned char opcode)
{
	cycle();
	unsigned char *_regtab[] = {
		&_registers.b,
		&_registers.c,
		&_registers.d,
		&_registers.e,
		&_registers.h,
		&_registers.l,
		0,
		&_registers.a };
	unsigned char *reg = _regtab[Z(opcode)];
	unsigned char val = reg ? *reg : _mmu->accessAt(_registers.hl);
	val |= 1 << Y(opcode);
	if (reg)
		*reg = val;
	else
	{
		cycle();
		_mmu->writeTo(_registers.hl, val);
		cycle();
	}
}

void	cpu::res(unsigned char opcode)
{
	cycle();
	unsigned char *_regtab[] = {
		&_registers.b,
		&_registers.c,
		&_registers.d,
		&_registers.e,
		&_registers.h,
		&_registers.l,
		0,
		&_registers.a };
	unsigned char *reg = _regtab[Z(opcode)];
	unsigned char val = reg ? *reg : _mmu->accessAt(_registers.hl);
	unsigned char bit = 1 << Y(opcode);
	if (val & bit)
		val ^= bit;
	if (reg)
		*reg = val;
	else
	{
		cycle();
		_mmu->writeTo(_registers.hl, val);
		cycle();
	}
}

void	cpu::jp(unsigned short addr)
{
	_registers.pc = addr;
}

void	cpu::jp(unsigned short addr, unsigned short ye)
{
	if (ye)
	{
		cycle();
		_registers.pc = addr;
	}
}

void	cpu::jr(char dis)
{
	cycle();
	unsigned short val = dis < 0 ? -dis : dis;
	_registers.pc = (dis < 0 ? _registers.pc - val : _registers.pc + val);
}

void	cpu::jr(char dis, unsigned short ye)
{
//	unsigned char ftab[] = {
//		_registers.f & bitflags::z ? 0 : 1,
//		_registers.f & bitflags::z ? 1 : 0,
//		_registers.f & bitflags::cy ? 0 : 1,
//		_registers.f & bitflags::cy ? 1 : 0};
	unsigned short val = dis < 0 ? -dis : dis;
	if (ye)
	{
		cycle();
		_registers.pc = (dis < 0 ? _registers.pc - val : _registers.pc + val);
	}
}

void	cpu::call(unsigned short addr)
{
	union address val;
	val.addr = _registers.pc;
	cycle();
	_mmu->writeTo(--_registers.sp, val.n2);
	cycle();
	_mmu->writeTo(--_registers.sp, val.n1);
	_registers.pc = addr;
	cycle();
}

void	cpu::call(unsigned short addr, unsigned char ye)
{
//	unsigned char ftab[] = {
//		_registers.f & bitflags::z ? 0 : 1,
//		_registers.f & bitflags::z ? 1 : 0,
//		_registers.f & bitflags::cy ? 0 : 1,
//		_registers.f & bitflags::cy ? 1 : 0};
	if (ye)
	{
		union address val;
		val.addr = _registers.pc;
		_mmu->writeTo(--_registers.sp, val.n2);
		_mmu->writeTo(--_registers.sp, val.n1);
		_registers.pc = addr;
	}
	cycle();
}

void	cpu::rst(unsigned short addr)
{
	union address val;
	cycle();
	val.addr = _registers.pc;
	cycle();
	_mmu->writeTo(--_registers.sp, val.n2);
	cycle();
	_mmu->writeTo(--_registers.sp, val.n1);
	cycle();
	_registers.pc = addr;
	cycle();
	cycle();
	cycle();
//	PRINT_DEBUG("rst to %hx", addr);
//	exit(1);
}

void	cpu::ret(void)
{
	union address addr;
	cycle();
	addr.n1 = _mmu->accessAt(_registers.sp++);
	cycle();
	addr.n2 = _mmu->accessAt(_registers.sp++);
	_registers.pc = addr.addr;
	cycle();
}

void	cpu::ret(unsigned char ye)
{
//	unsigned char ftab[] = {
//		_registers.f & bitflags::z ? 0 : 1,
//		_registers.f & bitflags::z ? 1 : 0,
//		_registers.f & bitflags::cy ? 0 : 1,
//		_registers.f & bitflags::cy ? 1 : 0};
	if (ye)
	{
		union address addr;
		cycle();
		addr.n1 = _mmu->accessAt(_registers.sp++);
		cycle();
		addr.n2 = _mmu->accessAt(_registers.sp++);
		cycle();
		_registers.pc = addr.addr;
	}
	cycle();
}

void	cpu::reti(void)
{
	union address addr;
	cycle();
	addr.n1 = _mmu->accessAt(_registers.sp++);
	cycle();
	addr.n2 = _mmu->accessAt(_registers.sp++);
	_registers.pc = addr.addr;
	this->_ime = 1;
	cycle();
//	_mmu->writeTo(0xFFFF, 0x1F);
}

unsigned char	cpu::opcode_parse()
{
	_mmu->setINTS();
	cyc = interrupt_check();
	cycle();
	if (_halt == true)
		return 4;
	unsigned char opcode = _mmu->accessAt(_registers.pc);
	printf("pc: 0x%04hx opcode 0x%02hhx\n", _registers.pc, opcode);
	unsigned char ftab[4];
	_registers.pc += haltcheck;
	if (!haltcheck)
		haltcheck = 1;
	ftab[0] = _registers.f & bitflags::z ? 0 : 1;
	ftab[1] = _registers.f & bitflags::z ? 1 : 0;
	ftab[2] = _registers.f & bitflags::cy ? 0 : 1;
	ftab[3] = _registers.f & bitflags::cy ? 1 : 0;
	unsigned char *_regtab[] = {
		&_registers.b,
		&_registers.c,
		&_registers.d,
		&_registers.e,
		&_registers.h,
		&_registers.l,
		0,
		&_registers.a };
	unsigned short *_pairtabddss[] = {
		&_registers.bc,
		&_registers.de,
		&_registers.hl,
		&_registers.sp};
	unsigned short *_pairtabqq[] = {
		&_registers.bc,
		&_registers.de,
		&_registers.hl,
		&_registers.af};
	if (opcode == 0xCB)
	{
		opcode = _mmu->accessAt(_registers.pc++);
		printf("cb opcode 0x%02hhx\n", opcode);
		switch(X(opcode))
		{
			case 0:
				rot(Y(opcode), _regtab[Z(opcode)]);
				break;
			case 1:
				bit(opcode);
				break;
			case 2:
				res(opcode);
				break;
			case 3:
				set(opcode);
				break;
			default:
				exit(1);
		}
		_registers.f &= 0xF0;
		return cyc;
	}
	if (!opcode)
		return cyc;
	if (opcode < 0x3F && ((opcode & 0x0F) == 0x06 || (opcode & 0x0F) == 0x0E))//LD r, n
	{
	    ld(_regtab[Y(opcode)], _mmu->accessAt(_registers.pc++));
	    cycle();
	}
	else if (0x40 <= opcode && opcode <= 0x7F)//LD r, r 0x76 HALT
	{
		unsigned char *reg = _regtab[Z(opcode)];
		if (!reg)
		{
			cycle();
		}
		if (opcode == 0x76)
			halt();
		else
		{
			ld(_regtab[Y(opcode)], reg ? *reg : _mmu->accessAt(_registers.hl));
		}
	}
	else if (opcode >= 0xE0 && ((opcode & 0x0F) == 0x0A || (opcode & 0x0F) == 0x02 || !(opcode & 0x0F)))//LD ff00 + c/n or nn to/from A
	{
		union address addr;
		if ((opcode & 0x0F) == 0x0A)
		{
			addr.n1 = _mmu->accessAt(_registers.pc++);
			cycle();
			addr.n2 = _mmu->accessAt(_registers.pc++);
			cycle();
		}
		else
		{
			if (opcode & 0x0F)
				addr.addr = 0xFF00 + _registers.c;
			else
			{
				addr.addr = 0xFF00 + _mmu->accessAt(_registers.pc++);
				cycle();
			}
		}
		if ((opcode & 0xF0) == 0xF0)
		{
			ld(&_registers.a, _mmu->accessAt(addr.addr));
			cycle();
		}
		else
			ld(addr, _registers.a);
	}
	else if (X(opcode) == 0 && Z(opcode) == 2)//LD reg pair into/out of a. dec/inc hl if needed
	{
		union address addr;
		switch(P(opcode))
		{
			case 0:
				addr.addr = _registers.bc;
				break;
			case 1:
				addr.addr = _registers.de;
				break;
			case 2:
				addr.addr = _registers.hl;
				_registers.hl = _registers.hl + 1;
				break;
			case 3:
				addr.addr = _registers.hl;
				_registers.hl = _registers.hl - 1;
				break;
		}
		if (Q(opcode))
			ld(&_registers.a, addr.addr);
		else
			ld(addr, _registers.a);
	}
	else if (X(opcode) == 0 && Z(opcode) == 1)
	{
		if (!(Q(opcode)))//LD rr, nn
		{
			union address addr;
			cycle();
			addr.n1 = _mmu->accessAt(_registers.pc++);
			cycle();
			addr.n2 = _mmu->accessAt(_registers.pc++);
			ld(_pairtabddss[P(opcode)], addr.addr);
		}
		else //ADD HL, rr
			add(_pairtabddss[P(opcode)]);
	}
	else if (opcode == 0xF8)//LDHL SP, n
	{
		cycle();
		char dis = _mmu->accessAt(_registers.pc++);
		cycle();
		ldhl(dis);
	}
	else if (opcode == 0xF9) //LD SP, HL
	{
		cycle();
		ld(&_registers.sp, _registers.hl);
	}
   else if (opcode == 0x08)//LD (nn), SP
   {
	   union address addr;
	   union address sp;
		cycle();
	   addr.n1 = _mmu->accessAt(_registers.pc++);
		cycle();
	   addr.n2 = _mmu->accessAt(_registers.pc++);
		cycle();
	   sp.addr = _registers.sp;
	   ld(addr, sp);
   }
	else if (opcode > 0xC0 && (opcode & 0x0F) == 5)//PUSH
		push(_pairtabqq[P(opcode)]);
	else if (opcode > 0xC0 && (opcode & 0x0F) == 1)//POP
		pop(_pairtabqq[P(opcode)]);
	else if (0x80 <= opcode && opcode <= 0xBF)//ALU r
	{
		unsigned char *regp = _regtab[Z(opcode)];
		unsigned char val = regp ? *regp : _mmu->accessAt(_registers.hl);
		if (!regp)
			cycle();
		alu(Y(opcode), val);
	}
	else if (X(opcode) == 3 && Z(opcode) == 6)//ALU n
	{
		alu(Y(opcode), _mmu->accessAt(_registers.pc++));
		cycle();
	}
	else if (X(opcode) == 0 && 3 <= Z(opcode) && Z(opcode) <= 5)//INC/DEC rr/r
		switch(Z(opcode))
		{
			case 3:
				Q(opcode) ? dec(_pairtabddss[P(opcode)]) : inc(_pairtabddss[P(opcode)]);
				break;
			case 4:
				inc(_regtab[Y(opcode)]);
				break;
			case 5:
				dec(_regtab[Y(opcode)]);
				break;
		}
	else if (opcode == 0xE8)//ADD SP
	{
		char val = _mmu->accessAt(_registers.pc++);	
		cycle();
		add(val);
	}
	else if (X(opcode) == 0 && Z(opcode) == 7)//RLCA RRCA RLA RRA DAA CPL SCF CCF on A
		acctab(Y(opcode));
	else if (opcode == 0x10)//STOP
		stop();
	else if (opcode == 0xF3)//DI
		di();
	else if (opcode == 0xFB)//EI
		ei();
	else if (X(opcode) == 0	&& Z(opcode) == 0 && Y(opcode) >=3)//JR d/JR cc, d
	{
		char val = _mmu->accessAt(_registers.pc++);
		cycle();
//		PRINT_DEBUG("jr: y %d ftab %hhd", Y(opcode), ftab[Y(opcode) - 4]);
	//	sleep(1);
		(Y(opcode) == 3) ? jr(val) : jr(val, ftab[Y(opcode) - 4]);
	}
	else if (opcode == 0xE9)//JP HL
		jp(_registers.hl);
	else if (X(opcode) == 3 && (Z(opcode) == 2 || Z(opcode) == 3))//JP nn/JP cc, nn 
	{
		union address addr;
		cycle();
		addr.n1 = _mmu->accessAt(_registers.pc++);
		cycle();
		addr.n2 = _mmu->accessAt(_registers.pc++);
		if (Z(opcode) == 3)
		{
			cycle();
			jp(addr.addr);
		}
		else
			jp(addr.addr, ftab[Y(opcode)]);
	}
	else if (X(opcode) == 3 && (Z(opcode) == 4 || Z(opcode) == 5))//CALL nn/CALL cc, nn
	{
		union address addr;
		addr.n1 = _mmu->accessAt(_registers.pc++);
		cycle();
		addr.n2 = _mmu->accessAt(_registers.pc++);
		cycle();
		Z(opcode) == 5 ? call(addr.addr) : call(addr.addr, ftab[Y(opcode)]);
	}
	else if (X(opcode) == 3 && Z(opcode) == 7)//RST
		rst(Y(opcode) * 8);
	else if (X(opcode) == 3 && !Z(opcode))//RET cc
		ret(ftab[Y(opcode)]);
	else if (opcode == 0xC9)//RET
		ret();
	else if (opcode == 0xD9)//RETI
		reti();
	else
	{
		std::cerr << "Unhandled opcode: 0x" << std::hex << +opcode << std::endl;
		exit(1);
	}
	_registers.f &= 0xF0;
	return cyc;
}
