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

// make sure rotations have to use the carry flag or not
// todo: implement a method of handling system cycles
// so that cpu and ppu can work together properly

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
//		printf("interrupt checking\n");
		unsigned short	targets[] = {
			0x40, 0x48, 0x50, 0x58, 0x60};
		unsigned char 	intif = _mmu->accessAt(0xFF0F);
		int c;
		for (c = 1; c <= 0x1F && !(c & intif); c <<= 1)
			i++;
		if (c <= 0x1F && (c & _mmu->accessAt(0xFFFF)))
		{
//			printf("jump to 0x%02x\n", targets[i]);
			if (_ime)
			{
				_mmu->writeTo(0xFF0F, intif & ~c);
				_ime = 0;
				call(targets[i]);
			}
			if (_halt == true)
			{
				_halt = false;
			   return opcode_parse(_ime ? 1 : 0);	
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

void	cpu::stop(void)//check p1 bits and p1 line
{
	while ((_mmu->accessAt(0xFF00) & 0xFF) == 0xFF)
		continue;
}

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
//	printf("rst to %hx\n", addr);
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

unsigned char	cpu::opcode_parse(void)
{
	return opcode_parse(1);
}

unsigned char _cbtab[] ={
	8,			//{"RLC", "B"},
	8,			//{"RLC", "C"},
	8,			//{"RLC", "D"},
	8,			//{"RLC", "E"},
	8,			//{"RLC", "H"},
	8,			//{"RLC", "L"},
	16,			//{"RLC", "(HL)"},
	8,			//{"RLC", "A"},
	8,			//{"RRC", "B"},
	8,			//{"RRC", "C"},
	8,			//{"RRC", "D"},
	8,			//{"RRC", "E"},
	8,			//{"RRC", "H"},
	8,			//{"RRC", "L"},
	16,			//{"RRC", "(HL)"},
	8,			//{"RRC", "A"},
	8,			//{"RL", "B"},
	8,			//{"RL", "C"},
	8,			//{"RL", "D"},
	8,			//{"RL", "E"},
	8,			//{"RL", "H"},
	8,			//{"RL", "L"},
	16,			//{"RL", "(HL)"},
	8,			//{"RL", "A"},
	8,			//{"RR", "B"},
	8,			//{"RR", "C"},
	8,			//{"RR", "D"},
	8,			//{"RR", "E"},
	8,			//{"RR", "H"},
	8,			//{"RR", "L"},
	16,			//{"RR", "(HL)"},
	8,			//{"RR", "A"},
	8,			//{"SLA", "B"},
	8,			//{"SLA", "C"},
	8,			//{"SLA", "D"},
	8,			//{"SLA", "E"},
	8,			//{"SLA", "H"},
	8,			//{"SLA", "L"},
	16,			//{"SLA", "(HL)"},
	8,			//{"SLA", "A"},
	8,			//{"SRA", "B"},
	8,			//{"SRA", "C"},
	8,			//{"SRA", "D"},
	8,			//{"SRA", "E"},
	8,			//{"SRA", "H"},
	8,			//{"SRA", "L"},
	16,			//{"SRA", "(HL)"},
	8,			//{"SRA", "A"},
	8,			//{"SWAP", "B"},
	8,			//{"SWAP", "C"},
	8,			//{"SWAP", "D"},
	8,			//{"SWAP", "E"},
	8,			//{"SWAP", "H"},
	8,			//{"SWAP", "L"},
	16,			//{"SWAP", "(HL)"},
	8,			//{"SWAP", "A"},
	8,			//{"SRL", "B"},
	8,			//{"SRL", "C"},
	8,			//{"SRL", "D"},
	8,			//{"SRL", "E"},
	8,			//{"SRL", "H"},
	8,			//{"SRL", "L"},
	16,			//{"SRL", "(HL)"},
	8,			//{"SRL", "A"},
	8,			//{"BIT", "0, B"},
	8,			//{"BIT", "0, C"},
	8,			//{"BIT", "0, D"},
	8,			//{"BIT", "0, E"},
	8,			//{"BIT", "0, H"},
	8,			//{"BIT", "0, L"},
	16,			//{"BIT", "0, (HL)"},
	8,			//{"BIT", "0, A"},
	8,			//{"BIT", "1, B"},
	8,			//{"BIT", "1, C"},
	8,			//{"BIT", "1, D"},
	8,			//{"BIT", "1, E"},
	8,			//{"BIT", "1, H"},
	8,			//{"BIT", "1, L"},
	16,			//{"BIT", "1, (HL)"},
	8,			//{"BIT", "1, A"},
	8,			//{"BIT", "2, B"},
	8,			//{"BIT", "2, C"},
	8,			//{"BIT", "2, D"},
	8,			//{"BIT", "2, E"},
	8,			//{"BIT", "2, H"},
	8,			//{"BIT", "2, L"},
	16,			//{"BIT", "2, (HL)"},
	8,			//{"BIT", "2, A"},
	8,			//{"BIT", "3, B"},
	8,			//{"BIT", "3, C"},
	8,			//{"BIT", "3, D"},
	8,			//{"BIT", "3, E"},
	8,			//{"BIT", "3, H"},
	8,			//{"BIT", "3, L"},
	16,			//{"BIT", "3, (HL)"},
	8,			//{"BIT", "3, A"},
	8,			//{"BIT", "4, B"},
	8,			//{"BIT", "4, C"},
	8,			//{"BIT", "4, D"},
	8,			//{"BIT", "4, E"},
	8,			//{"BIT", "4, H"},
	8,			//{"BIT", "4, L"},
	16,			//{"BIT", "4, (HL)"},
	8,			//{"BIT", "4, A"},
	8,			//{"BIT", "5, B"},
	8,			//{"BIT", "5, C"},
	8,			//{"BIT", "5, D"},
	8,			//{"BIT", "5, E"},
	8,			//{"BIT", "5, H"},
	8,			//{"BIT", "5, L"},
	16,			//{"BIT", "5, (HL)"},
	8,			//{"BIT", "5, A"},
	8,			//{"BIT", "6, B"},
	8,			//{"BIT", "6, C"},
	8,			//{"BIT", "6, D"},
	8,			//{"BIT", "6, E"},
	8,			//{"BIT", "6, H"},
	8,			//{"BIT", "6, L"},
	16,			//{"BIT", "6, (HL)"},
	8,			//{"BIT", "6, A"},
	8,			//{"BIT", "7, B"},
	8,			//{"BIT", "7, C"},
	8,			//{"BIT", "7, D"},
	8,			//{"BIT", "7, E"},
	8,			//{"BIT", "7, H"},
	8,			//{"BIT", "7, L"},
	16,			//{"BIT", "7, (HL)"},
	8,			//{"BIT", "7, A"},
	8,			//{"RES", "0, B"},
	8,			//{"RES", "0, C"},
	8,			//{"RES", "0, D"},
	8,			//{"RES", "0, E"},
	8,			//{"RES", "0, H"},
	8,			//{"RES", "0, L"},
	16,			//{"RES", "0, (HL)"},
	8,			//{"RES", "0, A"},
	8,			//{"RES", "1, B"},
	8,			//{"RES", "1, C"},
	8,			//{"RES", "1, D"},
	8,			//{"RES", "1, E"},
	8,			//{"RES", "1, H"},
	8,			//{"RES", "1, L"},
	16,			//{"RES", "1, (HL)"},
	8,			//{"RES", "1, A"},
	8,			//{"RES", "2, B"},
	8,			//{"RES", "2, C"},
	8,			//{"RES", "2, D"},
	8,			//{"RES", "2, E"},
	8,			//{"RES", "2, H"},
	8,			//{"RES", "2, L"},
	16,			//{"RES", "2, (HL)"},
	8,			//{"RES", "2, A"},
	8,			//{"RES", "3, B"},
	8,			//{"RES", "3, C"},
	8,			//{"RES", "3, D"},
	8,			//{"RES", "3, E"},
	8,			//{"RES", "3, H"},
	8,			//{"RES", "3, L"},
	16,			//{"RES", "3, (HL)"},
	8,			//{"RES", "3, A"},
	8,			//{"RES", "4, B"},
	8,			//{"RES", "4, C"},
	8,			//{"RES", "4, D"},
	8,			//{"RES", "4, E"},
	8,			//{"RES", "4, H"},
	8,			//{"RES", "4, L"},
	16,			//{"RES", "4, (HL)"},
	8,			//{"RES", "4, A"},
	8,			//{"RES", "5, B"},
	8,			//{"RES", "5, C"},
	8,			//{"RES", "5, D"},
	8,			//{"RES", "5, E"},
	8,			//{"RES", "5, H"},
	8,			//{"RES", "5, L"},
	16,			//{"RES", "5, (HL)"},
	8,			//{"RES", "5, A"},
	8,			//{"RES", "6, B"},
	8,			//{"RES", "6, C"},
	8,			//{"RES", "6, D"},
	8,			//{"RES", "6, E"},
	8,			//{"RES", "6, H"},
	8,			//{"RES", "6, L"},
	16,			//{"RES", "6, (HL)"},
	8,			//{"RES", "6, A"},
	8,			//{"RES", "7, B"},
	8,			//{"RES", "7, C"},
	8,			//{"RES", "7, D"},
	8,			//{"RES", "7, E"},
	8,			//{"RES", "7, H"},
	8,			//{"RES", "7, L"},
	16,			//{"RES", "7, (HL)"},
	8,			//{"RES", "7, A"},
	8,			//{"SET", "0, B"},
	8,			//{"SET", "0, C"},
	8,			//{"SET", "0, D"},
	8,			//{"SET", "0, E"},
	8,			//{"SET", "0, H"},
	8,			//{"SET", "0, L"},
	16,			//{"SET", "0, (HL)"},
	8,			//{"SET", "0, A"},
	8,			//{"SET", "1, B"},
	8,			//{"SET", "1, C"},
	8,			//{"SET", "1, D"},
	8,			//{"SET", "1, E"},
	8,			//{"SET", "1, H"},
	8,			//{"SET", "1, L"},
	16,			//{"SET", "1, (HL)"},
	8,			//{"SET", "1, A"},
	8,			//{"SET", "2, B"},
	8,			//{"SET", "2, C"},
	8,			//{"SET", "2, D"},
	8,			//{"SET", "2, E"},
	8,			//{"SET", "2, H"},
	8,			//{"SET", "2, L"},
	16,			//{"SET", "2, (HL)"},
	8,			//{"SET", "2, A"},
	8,			//{"SET", "3, B"},
	8,			//{"SET", "3, C"},
	8,			//{"SET", "3, D"},
	8,			//{"SET", "3, E"},
	8,			//{"SET", "3, H"},
	8,			//{"SET", "3, L"},
	16,			//{"SET", "3, (HL)"},
	8,			//{"SET", "3, A"},
	8,			//{"SET", "4, B"},
	8,			//{"SET", "4, C"},
	8,			//{"SET", "4, D"},
	8,			//{"SET", "4, E"},
	8,			//{"SET", "4, H"},
	8,			//{"SET", "4, L"},
	16,			//{"SET", "4, (HL)"},
	8,			//{"SET", "4, A"},
	8,			//{"SET", "5, B"},
	8,			//{"SET", "5, C"},
	8,			//{"SET", "5, D"},
	8,			//{"SET", "5, E"},
	8,			//{"SET", "5, H"},
	8,			//{"SET", "5, L"},
	16,			//{"SET", "5, (HL)"},
	8,			//{"SET", "5, A"},
	8,			//{"SET", "6, B"},
	8,			//{"SET", "6, C"},
	8,			//{"SET", "6, D"},
	8,			//{"SET", "6, E"},
	8,			//{"SET", "6, H"},
	8,			//{"SET", "6, L"},
	16,			//{"SET", "6, (HL)"},
	8,			//{"SET", "6, A"},
	8,			//{"SET", "7, B"},
	8,			//{"SET", "7, C"},
	8,			//{"SET", "7, D"},
	8,			//{"SET", "7, E"},
	8,			//{"SET", "7, H"},
	8,			//{"SET", "7, L"},
	16,			//{"SET", "7, (HL)"},
	8,			//{"SET", "7, A"}
};

unsigned char	cpu::opcode_parse(unsigned char haltcheck)
{
	_mmu->setINTS();
	cyc = interrupt_check();
	cycle();
	if (_halt == true)
		return 4;
	unsigned char opcode = _mmu->accessAt(_registers.pc);
//	if (debug == true)
//	printf("\t\tcurrent pc: 0x%04x\n", _registers.pc);
//	printf("A:%02hhX F:%C%C%C%C BC:%04X DE:%04x HL:%04x SP:%04x PC:%04x\n", _registers.a, _registers.f & 0x80 ? 'Z' : '-', _registers.f & 0x40 ? 'N' : '-', _registers.f & 0x20 ? 'H' : '-', _registers.f & 0x10 ? 'C' : '-', _registers.bc, _registers.de, _registers.hl, _registers.sp, _registers.pc);
	unsigned char ftab[4];
	_registers.pc += haltcheck;
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
//	if (debug == true)
//		printf("\tnz %d z %d nc %d c %d\n\n", ftab[0], ftab[1], ftab[2], ftab[3]);
	if (opcode == 0xCB)
	{
		opcode = _mmu->accessAt(_registers.pc++);
//		if (debug == true)
//			debug_print(opcode, 1, _registers, _mmu->PaccessAt(0xFF40), _mmu->PaccessAt(0xFF41), _mmu->PaccessAt(0xFF45), _mmu->PaccessAt(0xFF44), _mmu->PaccessAt(0xFF0F), _mmu->PaccessAt(0xFFFF), _ime);
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
//	if (debug == true)
//		debug_print(opcode, 0, _registers, _mmu->PaccessAt(0xFF40), _mmu->PaccessAt(0xFF41), _mmu->PaccessAt(0xFF45), _mmu->PaccessAt(0xFF44), _mmu->PaccessAt(0xFF0F), _mmu->PaccessAt(0xFFFF), _ime);
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
		if ((opcode & 0xF0) == 0xF0)
		{
			ld(&_registers.a, _mmu->accessAt(addr.addr));
			cycle();
		}
		else
			ld(addr, _registers.a);
		}
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
	{
		acctab(Y(opcode));
	}
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
//		printf("jr: y %d ftab %hhd\n", Y(opcode), ftab[Y(opcode) - 4]);
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
//	unsigned char c;
//	if (_registers.pc >= 0x36C && _registers.pc <= 0x36F)
//		debug = true;
//	if (debug)
//	{
//		printf("step\n");
//		read(0, &c, 1);
//	}
//	usleep(100000);
	_registers.f &= 0xF0;
	return cyc;
}
