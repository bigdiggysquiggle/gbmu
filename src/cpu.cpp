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

cpu::cpu(std::shared_ptr<mmu> unit, std::shared_ptr<ppu> pp)
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
	_mmu = unit;
	_ppu = pp;
	haltcheck = 1;
	_inCycles = 0;
	ime_set = 0;
	_ime = 0;
}

bool	cpu::imeCheck()
{
	if (ime_set)
	{
		_ime = (ime_set % 2);
		ime_set = 0;
		return true;
	}
	return false;
}

uint8_t	nLogo[]={
	0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03,
	0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D, 0x00, 0x08,
	0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E,
	0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63,
	0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB,
	0xB9, 0x33, 0x3E};

/*void	cpu::checkRom(void)
{
	uint16_t start = 0x104;
	uint16_t end = _registers.a == 0x01 ? 0x133 : 0x11C;
	while (start <= end && nLogo[start - 0x104] == _mmu->accessAt(start))
		start++;
	if (start <= end)
		throw "Error: invalid cart";
	uint8_t modecheck = _mmu->accessAt(0x143);
	if (_registers.a == 0x11 && (modecheck == 0x80 || modecheck == 0xC0))
		_mmu->_cgb_mode = 0x03;

}*/

uint8_t	cpu::interrupt_check(void)
{
	int i = 0;
	imeCheck();
	if (_halt == true || _ime)
	{
//		PRINT_DEBUG("interrupt checking");
		uint16_t	targets[] = {
			0x40, 0x48, 0x50, 0x58, 0x60};
		uint8_t 	intif = _mmu->accessAt(0xFF0F);
		uint8_t c;
		if (c <= 0x1F && (c & _mmu->accessAt(0xFFFF)))
		{
//			PRINT_DEBUG("jump to 0x%02x", targets[i]);
			if (_ime)
			{
				_mmu->writeTo(0xFF0F, intif & ~c);
				_ime = 0;
				union address val;
				val.addr = _registers.pc;
				_mmu->writeTo(--_registers.sp, val.n2);
				_mmu->writeTo(--_registers.sp, val.n1);
				_registers.pc = targets[i];
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

// make this also reset the register that tells the console
// to not use the bootrom

void	cpu::reset()
{
	_registers.pc = 0;
	_mmu->writeTo(0xFF40, 0x00);
	_mmu->writeTo(0xFF50, 0x00);
}

void	cpu::setInterrupt(uint8_t INT)
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

void	cpu::ld(uint8_t *reg, uint8_t val)
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

void	cpu::ld(uint8_t *regd, uint16_t addr)
{
	if (regd)
	{
		cycle();
		*regd = _mmu->accessAt(addr);
	}
	else
	{
		cycle();
		_mmu->writeTo(_registers.hl, _mmu->accessAt(addr));
	}
}

void	cpu::ldd(uint8_t a)
{
	if (a)
		_registers.a = _mmu->accessAt(_registers.hl);
	else
		_mmu->writeTo(_registers.hl, _registers.a);
	cycle();
	_registers.hl = _registers.hl - 1;
}

void	cpu::ldi(uint8_t a)
{
	if (a)
		_registers.a = _mmu->accessAt(_registers.hl);
	else
		_mmu->writeTo(_registers.hl, _registers.a);
	cycle();
	_registers.hl++;
}

void	cpu::ld(uint16_t *regp, uint16_t val)
{
	*regp = val;
}

void	cpu::ld(union address addr, uint8_t val)
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
	uint16_t dis;
	uint8_t uns = n;
	uint32_t res;
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

void	cpu::push(uint16_t *regp)
{
	union address val;
	val.addr = *regp;
	cycle();
	_mmu->writeTo(--_registers.sp, val.n2);
	cycle();
	_mmu->writeTo(--_registers.sp, val.n1);
	cycle();
}

void	cpu::pop(uint16_t *regp)
{
	union address val;//endian is gonna kill me I swear
	cycle();
	val.n1 = _mmu->accessAt(_registers.sp++);
	cycle();
	val.n2 = _mmu->accessAt(_registers.sp++);
	*regp = val.addr;
}

void	cpu::add(uint16_t *regp)
{
	cycle();
	uint32_t res = *regp + _registers.hl;
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
	uint16_t dis;
	uint8_t uns = val;
	uint32_t res;
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

void	cpu::add(uint8_t val)
{
	uint16_t res = _registers.a + val;
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

void	cpu::adc(uint8_t val)
{
	uint8_t c = (_registers.f & bitflags::cy) ? 1 : 0;
	uint16_t res = _registers.a + val + c;
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

void	cpu::sub(uint8_t val)
{
	uint16_t res = _registers.a - val;
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

void	cpu::sbc(uint8_t val)
{
	uint8_t c = (_registers.f & bitflags::cy) ? 1 : 0;
	uint16_t res = _registers.a - (val + c);
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

void	cpu::_or(uint8_t val)
{
	_registers.a |= val;
	_registers.f &= ~(bitflags::n + bitflags::cy + bitflags::h);
	if (!_registers.a)
		_registers.f |= bitflags::z;
	else
		_registers.f &= ~(bitflags::z);
}

void	cpu::_xor(uint8_t val)
{
	_registers.a ^= val;
	_registers.f &= ~(bitflags::n + bitflags::cy + bitflags::h);
	if (!_registers.a)
		_registers.f |= bitflags::z;
	else
		_registers.f &= ~(bitflags::z);
}

void	cpu::_and(uint8_t val)
{
	_registers.a &= val;
	_registers.f &= ~(bitflags::n | bitflags::cy);
	_registers.f |= bitflags::h;
	if (!_registers.a)
		_registers.f |= bitflags::z;
	else
		_registers.f &= ~(bitflags::z);
}

void	cpu::cp(uint8_t val)
{
	uint16_t res;

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

void	cpu::inc(uint8_t *reg)
{
	uint8_t val;
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

void	cpu::inc(uint16_t *regp)
{
	cycle();
	*regp += 1;
}

void	cpu::dec(uint8_t *reg)
{
	uint8_t val;
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

void	cpu::dec(uint16_t *regp)
{
	*regp = *regp - 1;
}

void	cpu::swap(uint8_t *reg)
{
	cycle();
	uint8_t val;
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
//TODO: reimplement in a manner similar to halt.
void	cpu::stop(void)//TODO: check p1 bits and p1 line
{
	while ((_mmu->accessAt(0xFF00) & 0xFF) == 0xFF)
		continue;
}
/*
uint8_t	cpu::di(void)
{
	cyc += opcode_parse();
	this->_ime = 0;
	return cyc;
}

uint8_t	cpu::ei(void)
{
	cyc += opcode_parse();
	this->_ime = 1;
	return cyc;
}
*/

uint8_t	cpu::di()
{
	ime_set = 2;
	return 0;
}

uint8_t	cpu::ei()
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
	uint8_t cb = (_registers.f >> 4) & 0x01;
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
	uint8_t cb = (_registers.f >> 4) & 0x01;
	_registers.f = (_registers.a & 0x01) ? (_registers.f | bitflags::cy) : (_registers.f & ~(bitflags::cy));
	_registers.a = (cb << 7) | (_registers.a >> 1);
	_registers.f &= ~(bitflags::n + bitflags::h + bitflags::z);
}

void	cpu::rlc(uint8_t *reg)
{
	cycle();
	uint8_t val;
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

void	cpu::rl(uint8_t *reg)
{
	cycle();
	uint8_t cb = (_registers.f >> 4) & 0x01;
	uint8_t val;
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

void	cpu::rrc(uint8_t *reg)
{
	cycle();
	uint8_t val;
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

void	cpu::rr(uint8_t *reg)
{
	cycle();
	uint8_t cb = (_registers.f >> 4) & 0x01;
	uint8_t val;
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

void	cpu::sla(uint8_t *reg)
{
	cycle();
	uint8_t val;
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

void	cpu::sra(uint8_t *reg)
{
	cycle();
	uint8_t val;
	val = reg ? *reg : _mmu->accessAt(_registers.hl);
	uint8_t bit = (val & 0x80);
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

void	cpu::srl(uint8_t *reg)
{
	cycle();
	uint8_t val;
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

void	cpu::bit(uint8_t opcode)
{
	cycle();
	uint8_t *_regtab[] = {
		&_registers.b,
		&_registers.c,
		&_registers.d,
		&_registers.e,
		&_registers.h,
		&_registers.l,
		0,
		&_registers.a };
	uint8_t bit = 1 << Y(opcode);
	uint8_t *reg = _regtab[Z(opcode)];
	uint8_t val = reg ? *reg : _mmu->accessAt(_registers.hl);
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

void	cpu::set(uint8_t opcode)
{
	cycle();
	uint8_t *_regtab[] = {
		&_registers.b,
		&_registers.c,
		&_registers.d,
		&_registers.e,
		&_registers.h,
		&_registers.l,
		0,
		&_registers.a };
	uint8_t *reg = _regtab[Z(opcode)];
	uint8_t val = reg ? *reg : _mmu->accessAt(_registers.hl);
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

void	cpu::res(uint8_t opcode)
{
	cycle();
	uint8_t *_regtab[] = {
		&_registers.b,
		&_registers.c,
		&_registers.d,
		&_registers.e,
		&_registers.h,
		&_registers.l,
		0,
		&_registers.a };
	uint8_t *reg = _regtab[Z(opcode)];
	uint8_t val = reg ? *reg : _mmu->accessAt(_registers.hl);
	uint8_t bit = 1 << Y(opcode);
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

void	cpu::jp(uint16_t addr)
{
	_registers.pc = addr;
}

void	cpu::jp(uint16_t addr, uint16_t ye)
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
	uint16_t val = dis < 0 ? -dis : dis;
	_registers.pc = (dis < 0 ? _registers.pc - val : _registers.pc + val);
}

void	cpu::jr(char dis, uint16_t ye)
{
//	uint8_t ftab[] = {
//		_registers.f & bitflags::z ? 0 : 1,
//		_registers.f & bitflags::z ? 1 : 0,
//		_registers.f & bitflags::cy ? 0 : 1,
//		_registers.f & bitflags::cy ? 1 : 0};
	uint16_t val = dis < 0 ? -dis : dis;
	if (ye)
	{
		cycle();
		_registers.pc = (dis < 0 ? _registers.pc - val : _registers.pc + val);
	}
}

void	cpu::call(uint16_t addr)
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

void	cpu::call(uint16_t addr, uint8_t ye)
{
//	uint8_t ftab[] = {
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

void	cpu::rst(uint16_t addr)
{
	union address val;
	val.addr = _registers.pc;
	cycle();
	_mmu->writeTo(--_registers.sp, val.n2);
	cycle();
	_mmu->writeTo(--_registers.sp, val.n1);
	cycle();
	_registers.pc = addr;
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

void	cpu::ret(uint8_t ye)
{
//	uint8_t ftab[] = {
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

uint8_t	cpu::opcode_parse()
{
	_inCycles = 0;
	_mmu->setINTS();
	cyc = interrupt_check();
	cycle();
//	printf("haltcheck %d ", haltcheck);
	if (_halt == true)
		return 4;
	uint8_t opcode = _mmu->accessAt(_registers.pc);
//	printf("pc: 0x%04hx opcode 0x%02hhx\n", _registers.pc, opcode);
	_registers.pc += haltcheck;
	if (!haltcheck)
		haltcheck = 1;
	uint8_t *_regtab[] = {
		&_registers.b,
		&_registers.c,
		&_registers.d,
		&_registers.e,
		&_registers.h,
		&_registers.l,
		0,
		&_registers.a };

	//I have given up and resigned myself to hand writing
	//a colossal switch statement, as it's my understanding
	//that the C++ compiler will optimize this into a
	//proper jump table instead of making me have to
	//figure out how to produce a working jump table of
	//member functions complete with their associated
	//arguments
	union address addr;
	char dis;
	char val;
	switch(opcode)
	{
		case 0x00: //NOP
			return cyc;
			break;

		case 0x01:	//ld bc,nn
			addr.n1 = _mmu->accessAt(_registers.pc++);
			cycle();
			addr.n2 = _mmu->accessAt(_registers.pc++);
			cycle();
			ld(&_registers.bc, addr.addr);
			break;

		case 0x02:	//ld [bc], a
			addr.addr = _registers.bc;
			ld(addr, _registers.a);
			break;

		case 0x03:	//inc bc
			inc(&_registers.bc);
			break;

		case 0x04:	//inc b
			inc(&_registers.b);
			break;

		case 0x05:	//dec b
			dec(&_registers.b);
			break;

		case 0x06:	//ld b,n
			ld(&_registers.b, _mmu->accessAt(_registers.pc++));
			cycle();
			break;

		case 0x07:	//rlca
			rlca();
			break;

		case 0x08:	//ld nn,sp
			cycle();
			addr.n1 = _mmu->accessAt(_registers.pc++);
			cycle();
			addr.n2 = _mmu->accessAt(_registers.pc++);
			cycle();
			ld(addr, _registers.sp);
			break;

		case 0x09:	//add hl,bc
			add(&_registers.bc);
			break;

		case 0x0a:	//ld a,[bc]
			ld(&_registers.a, _mmu->accessAt(_registers.bc));
			cycle();
			break;

		case 0x0b:	//dec bc
			cycle();
			dec(&_registers.bc);
			break;

		case 0x0c:	//inc c
			inc(&_registers.c);
			break;

		case 0x0d:	//dec c
			dec(&_registers.c);
			break;

		case 0x0e:	//ld c,n
			ld(&_registers.c, _mmu->accessAt(_registers.pc++));
			cycle();
			break;

		case 0x0f:	//rrca
			rrca();
			break;

		case 0x10:	//stop
			stop();
			break;

		case 0x11:	//ld de, nn
			cycle();
			addr.n1 = _mmu->accessAt(_registers.pc++);
			cycle();
			addr.n2 = _mmu->accessAt(_registers.pc++);
			ld(&_registers.de, addr.addr);
			break;

		case 0x12:	//ld [de],a
			addr.addr = _registers.de;
			ld(addr, _registers.a);
			break;

		case 0x13:	//inc de
			inc(&_registers.de);
			break;

		case 0x14:	//inc d
			inc(&_registers.d);
			break;

		case 0x15:	//dec d
			dec(&_registers.d);
			break;

		case 0x16:	//ld d,n
			ld(&_registers.d, _mmu->accessAt(_registers.pc++));
			cycle();
			break;

		case 0x17:	//rla
			rla();
			break;

		case 0x18:	//jr d
			addr.n1 = _mmu->accessAt(_registers.pc++);
			cycle();
			jr(addr.n1);
			break;

		case 0x19:	//add hl, de
			add(&_registers.de);
			break;

		case 0x1a:	//ld a, [de]
			addr.addr = _registers.de;
			ld(&_registers.a, addr.addr);
			break;

		case 0x1b:	//dec de
			dec(&_registers.de);
			cycle();
			break;

		case 0x1c:	//inc e
			inc(&_registers.e);
			break;

		case 0x1d:	//dec e
			dec(&_registers.e);
			break;

		case 0x1e:	//ld e,n
			ld(&_registers.e, _mmu->accessAt(_registers.pc++));
			cycle();
			break;

		case 0x1f:	//rra
			rra();
			break;

		case 0x20:	//jr nz, d
			cycle();
			jr(_mmu->accessAt(_registers.pc++), FNZ);
			break;

		case 0x21:	//ld hl, nn
			cycle();
			addr.n1 = _mmu->accessAt(_registers.pc++);
			cycle();
			addr.n2 = _mmu->accessAt(_registers.pc++);
			ld(&_registers.hl, addr.addr);
			break;

		case 0x22:	//ld [hl+], a
			addr.addr = _registers.hl;
			_registers.hl += 1;
			ld(addr, _registers.a);
			break;

		case 0x23:	//inc hl
			inc(&_registers.hl);
			break;

		case 0x24:	//inc h
			inc(&_registers.h);
			break;

		case 0x25:	//dec h
			dec(&_registers.hl);
			break;

		case 0x26:	//ld h, n
			ld(&_registers.h, _mmu->accessAt(_registers.pc++));
			cycle();
			break;

		case 0x27:	//daa
			daa();
			break;

		case 0x28:	//jr z, d
			cycle();
			jr(_mmu->accessAt(_registers.pc++), FZ);
			break;

		case 0x29:	//add hl, hl
			add(&_registers.hl);
			break;

		case 0x2a:	//ld a,[hl+]
			addr.addr = _registers.hl;
			_registers.hl += 1;
			ld(&_registers.a, addr.addr);
			break;

		case 0x2b:	//dec hl
			dec(&_registers.hl);
			cycle();
			break;

		case 0x2c:	//inc l
			inc(&_registers.l);
			break;

		case 0x2d:	//dec l
			dec(&_registers.l);
			break;

		case 0x2e:	//ld l, n
			ld(&_registers.l, _mmu->accessAt(_registers.pc++));
			cycle();
			break;

		case 0x2f:	//cpl
			cpl();
			break;

		case 0x30:	//jr nc, d
			jr(_mmu->accessAt(_registers.pc++), FNC);
			cycle();
			break;

		case 0x31:	//ld sp, nn
			cycle();
			addr.n1 = _mmu->accessAt(_registers.pc++);
			cycle();
			addr.n2 = _mmu->accessAt(_registers.pc++);
			ld(&_registers.sp, addr.addr);
			break;

		case 0x32:	//ld [hl-],a
			addr.addr = _registers.hl;
			_registers.hl -= 1;
			ld(addr, _registers.a);
			break;

		case 0x33:	//inc sp
			inc(&_registers.sp);
			break;

		case 0x34:	//inc [hl]
			inc((uint8_t *)NULL);
			//NULL is coded to handle the address at HL
			break;

		case 0x35:	//dec [hl]
			dec((uint8_t *)NULL);
			//NULL is coded to handle the address at HL
			break;

		case 0x36:	//ld [hl],n
			ld((uint8_t *)NULL, _mmu->accessAt(_registers.pc++));
			cycle();
			//NULL is coded to handle the address at HL
			break;

		case 0x37:	//scf
			scf();
			break;

		case 0x38:	//jr c, d
			cycle();
			jr(_mmu->accessAt(_registers.pc++), FC);
			break;

		case 0x39:	//add hl,sp
			add(&_registers.sp);
			break;

		case 0x3a:	//ld a,[hl-]
			addr.addr = _registers.hl;
			_registers.hl -= 1;
			ld(&_registers.a, addr.addr);
			break;

		case 0x3b:	//dec sp
			dec(&_registers.sp);
			break;

		case 0x3c:	//inc a
			inc(&_registers.a);
			break;

		case 0x3d:	//dec a
			dec(&_registers.a);
			break;

		case 0x3e:	//ld a, n
			ld(&_registers.a, _mmu->accessAt(_registers.pc++));
			cycle();
			break;

		case 0x3f:	//ccf
			ccf();
			break;

		case 0x40:	//ld b,b
			ld(&_registers.b, _registers.b);
			break;

		case 0x41:	//ld b,c
			ld(&_registers.b, _registers.c);
			break;

		case 0x42:	//ld b,d
			ld(&_registers.b, _registers.d);
			break;

		case 0x43:	//ld b,e
			ld(&_registers.b, _registers.e);
			break;

		case 0x44:	//ld b,h
			ld(&_registers.b, _registers.h);
			break;

		case 0x45:	//ld b,l
			ld(&_registers.b, _registers.l);
			break;

		case 0x46:	//ld b,[hl]
			ld(&_registers.b, _registers.hl);
			break;

		case 0x47:	//ld b,a
			ld(&_registers.b, _registers.a);
			break;

		case 0x48:	//ld c,b
			ld(&_registers.c, _registers.b);
			break;

		case 0x49:	//ld c,c
			ld(&_registers.c, _registers.c);
			break;

		case 0x4a:	//ld c,d
			ld(&_registers.c, _registers.d);
			break;

		case 0x4b:	//ld c,e
			ld(&_registers.c, _registers.e);
			break;

		case 0x4c:	//ld c,h
			ld(&_registers.c, _registers.h);
			break;

		case 0x4d:	//ld c,l
			ld(&_registers.c, _registers.l);
			break;

		case 0x4e:	//ld c,[hl]
			ld(&_registers.c, _registers.hl);
			break;

		case 0x4f:	//ld c,a
			ld(&_registers.c, _registers.a);
			break;

		case 0x50:
			ld(&_registers.d, _registers.b);
			break;

		case 0x51:
			ld(&_registers.d, _registers.c);
			break;

		case 0x52:
			ld(&_registers.d, _registers.d);
			break;

		case 0x53:
			ld(&_registers.d, _registers.e);
			break;

		case 0x54:
			ld(&_registers.d, _registers.h);
			break;

		case 0x55:
			ld(&_registers.d, _registers.l);
			break;

		case 0x56:
			ld(&_registers.d, _registers.hl);
			break;

		case 0x57:
			ld(&_registers.d, _registers.a);
			break;

		case 0x58:
			ld(&_registers.e, _registers.b);
			break;

		case 0x59:
			ld(&_registers.e, _registers.c);
			break;

		case 0x5a:
			ld(&_registers.e, _registers.d);
			break;

		case 0x5b:
			ld(&_registers.e, _registers.e);
			break;

		case 0x5c:
			ld(&_registers.e, _registers.h);
			break;

		case 0x5d:
			ld(&_registers.e, _registers.l);
			break;

		case 0x5e:
			ld(&_registers.e, _registers.hl);
			break;

		case 0x5f:
			ld(&_registers.e, _registers.a);
			break;

		case 0x60:
			ld(&_registers.h, _registers.b);
			break;

		case 0x61:
			ld(&_registers.h, _registers.c);
			break;

		case 0x62:
			ld(&_registers.h, _registers.d);
			break;

		case 0x63:
			ld(&_registers.h, _registers.e);
			break;

		case 0x64:
			ld(&_registers.h, _registers.h);
			break;

		case 0x65:
			ld(&_registers.h, _registers.l);
			break;

		case 0x66:
			ld(&_registers.h, _registers.hl);
			break;

		case 0x67:
			ld(&_registers.h, _registers.a);
			break;

		case 0x68:
			ld(&_registers.l, _registers.b);
			break;

		case 0x69:
			ld(&_registers.l, _registers.c);
			break;

		case 0x6a:
			ld(&_registers.l, _registers.d);
			break;

		case 0x6b:
			ld(&_registers.l, _registers.e);
			break;

		case 0x6c:
			ld(&_registers.l, _registers.h);
			break;

		case 0x6d:
			ld(&_registers.l, _registers.l);
			break;

		case 0x6e:
			ld(&_registers.l, _registers.hl);
			break;

		case 0x6f:
			ld(&_registers.l, _registers.a);
			break;

//NULL handles the address at HL
		case 0x70:
			ld((uint8_t *)NULL, _registers.b);
			break;

		case 0x71:
			ld((uint8_t *)NULL, _registers.c);
			break;

		case 0x72:
			ld((uint8_t *)NULL, _registers.d);
			break;

		case 0x73:
			ld((uint8_t *)NULL, _registers.e);
			break;

		case 0x74:
			ld((uint8_t *)NULL, _registers.h);
			break;

		case 0x75:
			ld((uint8_t *)NULL, _registers.l);
			break;

		case 0x76:
			halt();
			break;

		case 0x77:
			ld((uint8_t *)NULL, _registers.a);
			break;

		case 0x78:
			ld(&_registers.a, _registers.b);
			break;

		case 0x79:
			ld(&_registers.a, _registers.c);
			break;

		case 0x7a:
			ld(&_registers.a, _registers.d);
			break;

		case 0x7b:
			ld(&_registers.a, _registers.e);
			break;

		case 0x7c:
			ld(&_registers.a, _registers.h);
			break;

		case 0x7d:
			ld(&_registers.a, _registers.l);
			break;

		case 0x7e:
			ld(&_registers.a, _registers.hl);
			break;

		case 0x7f:
			ld(&_registers.a, _registers.a);
			break;

//ADD
		case 0x80:
			add(_registers.b);
			break;

		case 0x81:
			add(_registers.c);
			break;

		case 0x82:
			add(_registers.d);
			break;

		case 0x83:
			add(_registers.e);
			break;

		case 0x84:
			add(_registers.h);
			break;

		case 0x85:
			add(_registers.l);
			break;

		case 0x86:
			add(_mmu->accessAt(_registers.hl));
			cycle();
			break;

		case 0x87:
			add(_registers.a);
			break;

//ADC
		case 0x88:
			adc(_registers.b);
			break;

		case 0x89:
			adc(_registers.c);
			break;

		case 0x8a:
			adc(_registers.d);
			break;

		case 0x8b:
			adc(_registers.e);
			break;

		case 0x8c:
			adc(_registers.h);
			break;

		case 0x8d:
			adc(_registers.l);
			break;

		case 0x8e:
			adc(_mmu->accessAt(_registers.hl));
			cycle();
			break;

		case 0x8f:
			adc(_registers.a);
			break;

//SUB
		case 0x90:
			sub(_registers.b);
			break;

		case 0x91:
			sub(_registers.c);
			break;

		case 0x92:
			sub(_registers.d);
			break;

		case 0x93:
			sub(_registers.e);
			break;

		case 0x94:
			sub(_registers.h);
			break;

		case 0x95:
			sub(_registers.l);
			break;

		case 0x96:
			sub(_mmu->accessAt(_registers.hl));
			cycle();
			break;

		case 0x97:
			sub(_registers.a);
			break;

//SBC
		case 0x98:
			sbc(_registers.b);
			break;

		case 0x99:
			sbc(_registers.c);
			break;

		case 0x9a:
			sbc(_registers.d);
			break;

		case 0x9b:
			sbc(_registers.e);
			break;

		case 0x9c:
			sbc(_registers.h);
			break;

		case 0x9d:
			sbc(_registers.l);
			break;

		case 0x9e:
			sbc(_mmu->accessAt(_registers.hl));
			break;

		case 0x9f:
			sbc(_registers.a);
			break;

//AND
		case 0xa0:
			_and(_registers.b);
			break;

		case 0xa1:
			_and(_registers.c);
			break;

		case 0xa2:
			_and(_registers.d);
			break;

		case 0xa3:
			_and(_registers.e);
			break;

		case 0xa4:
			_and(_registers.h);
			break;

		case 0xa5:
			_and(_registers.l);
			break;

		case 0xa6:
			_and(_mmu->accessAt(_registers.hl));
			break;

		case 0xa7:
			_and(_registers.a);
			break;

//XOR
		case 0xa8:
			_xor(_registers.b);
			break;

		case 0xa9:
			_xor(_registers.c);
			break;

		case 0xaa:
			_xor(_registers.d);
			break;

		case 0xab:
			_xor(_registers.e);
			break;

		case 0xac:
			_xor(_registers.h);
			break;

		case 0xad:
			_xor(_registers.l);
			break;

		case 0xae:
			_xor(_mmu->accessAt(_registers.hl));
			cycle();
			break;

		case 0xaf:
			_xor(_registers.a);
			break;

//OR
		case 0xb0:
			_or(_registers.b);
			break;

		case 0xb1:
			_or(_registers.c);
			break;

		case 0xb2:
			_or(_registers.d);
			break;

		case 0xb3:
			_or(_registers.e);
			break;

		case 0xb4:
			_or(_registers.h);
			break;

		case 0xb5:
			_or(_registers.l);
			break;

		case 0xb6:
			_or(_mmu->accessAt(_registers.hl));
			cycle();
			break;

		case 0xb7:
			_or(_registers.a);
			break;

//CP
		case 0xb8:
			cp(_registers.b);
			break;

		case 0xb9:
			cp(_registers.c);
			break;

		case 0xba:
			cp(_registers.d);
			break;

		case 0xbb:
			cp(_registers.e);
			break;

		case 0xbc:
			cp(_registers.h);
			break;

		case 0xbd:
			cp(_registers.l);
			break;

		case 0xbe:
			cp(_mmu->accessAt(_registers.hl));
			cycle();
			break;

		case 0xbf:
			cp(_registers.a);
			break;

		case 0xc0:	//ret nz
			ret(FNZ);
			break;

		case 0xc1:	//pop bc
			pop(&_registers.bc);
			break;

		case 0xc2:	//jp nz, nn
			cycle();
			addr.n1 = _mmu->accessAt(_registers.pc++);
			cycle();
			addr.n2 = _mmu->accessAt(_registers.pc++);
			jp(addr.addr, FNZ);
			break;

		case 0xc3:	//jp nn
			cycle();
			addr.n1 = _mmu->accessAt(_registers.pc++);
			cycle();
			addr.n2 = _mmu->accessAt(_registers.pc++);
			cycle();
			jp(addr.addr);
			break;

		case 0xc4:	//call nz, nn
			addr.n1 = _mmu->accessAt(_registers.pc++);
			cycle();
			addr.n2 = _mmu->accessAt(_registers.pc++);
			call(addr.addr, FNZ);
			break;

		case 0xc5:	//push bc
			push(&_registers.bc);
			break;

		case 0xc6:	//add a, n
			add(_mmu->accessAt(_registers.pc++));
			cycle();
			break;

		case 0xc7:	//rst 0x00
			rst(0x00);
			break;

		case 0xc8:	//ret z
			ret(FZ);
			break;

		case 0xc9:	//ret
			ret();
			break;

		case 0xca:	//jp z, nn
			cycle();
			addr.n1 = _mmu->accessAt(_registers.pc++);
			cycle();
			addr.n2 = _mmu->accessAt(_registers.pc++);
			jp(addr.addr, FZ);
			break;

		case 0xcb:	//CB table
			opcode = _mmu->accessAt(_registers.pc++);
//			printf("cb opcode 0x%02hhx\n", opcode);
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
			break;

		case 0xcc:	//call z, nn
			addr.n1 = _mmu->accessAt(_registers.pc++);
			cycle();
			addr.n2 = _mmu->accessAt(_registers.pc++);
			call(addr.addr, FZ);
			break;

		case 0xcd:	//call nn
			addr.n1 = _mmu->accessAt(_registers.pc++);
			cycle();
			addr.n2 = _mmu->accessAt(_registers.pc++);
			cycle();
			call(addr.addr);
			break;

		case 0xce:	//adc a, n
			adc(_mmu->accessAt(_registers.pc++));
			cycle();
			break;

		case 0xcf:	//rst
			rst(0x08);
			break;

		case 0xd0:	//ret nc
			ret(FNC);
			break;

		case 0xd1:	//pop de
			pop(&_registers.de);
			break;

		case 0xd2:	//jp nc
			cycle();
			addr.n1 = _mmu->accessAt(_registers.pc++);
			cycle();
			addr.n2 = _mmu->accessAt(_registers.pc++);
			jp(addr.addr, FNC);
			break;

		case 0xd3:	//nop
			return cyc;
			break;

		case 0xd4:	//call nc, nn
			cycle();
			addr.n1 = _mmu->accessAt(_registers.pc++);
			cycle();
			addr.n2 = _mmu->accessAt(_registers.pc++);
			call(addr.addr, FNC);
			break;

		case 0xd5:	//push de
			push(&_registers.de);
			break;

		case 0xd6:	//sub a, n
			sub(_mmu->accessAt(_registers.pc++));
			cycle();
			break;

		case 0xd7:	//rst
			rst(0x10);
			break;

		case 0xd8:	//ret c
			ret(FC);
			break;

		case 0xd9:	//reti
			reti();
			break;

		case 0xda:	//jp c, nn
			cycle();
			addr.n1 = _mmu->accessAt(_registers.pc++);
			cycle();
			addr.n2 = _mmu->accessAt(_registers.pc++);
			jp(addr.addr, FC);
			break;

		case 0xdb:	//nop
			return cyc;
			break;

		case 0xdc:	//call c, nn
			cycle();
			addr.n1 = _mmu->accessAt(_registers.pc++);
			cycle();
			addr.n2 = _mmu->accessAt(_registers.pc++);
			call(addr.addr, FC);
			break;

		case 0xdd:	//nop
			return cyc;
			break;

		case 0xde:	//nop
			return cyc;
			break;

		case 0xdf:	//rst
			rst(0x18);
			break;

		case 0xe0:	//ldh [ff+n], a
			addr.addr = 0xFF00 + _mmu->accessAt(_registers.pc++);
			cycle();
			ld(addr, _registers.a);
			break;

		case 0xe1:	//pop hl
			pop(&_registers.hl);
			break;

		case 0xe2:	//ld [ff+c], a
			addr.addr = 0xFF00 + _registers.c;
			ld(addr, _registers.a);
			break;

		case 0xe3:	//nop
			return cyc;
			break;

		case 0xe4:	//nop
			return cyc;
			break;

		case 0xe5:	//push hl
			push(&_registers.hl);
			break;

		case 0xe6:	//and n
			_and(_mmu->accessAt(_registers.pc++));
			cycle();
			break;

		case 0xe7:	//rst
			rst(0x20);
			break;

		case 0xe8:	//add sp, n (signed)
			val = _mmu->accessAt(_registers.pc++);
			cycle();
			add(val);
			break;

		case 0xe9:	//jp [hl]
			jp(_registers.hl);
			break;

		case 0xea:	//ld [nn], a
			addr.n1 = _mmu->accessAt(_registers.pc++);
			cycle();
			addr.n2 = _mmu->accessAt(_registers.pc++);
			cycle();
			ld(addr, _registers.a);
			break;

		case 0xeb:	//nop
			return cyc;
			break;

		case 0xec:	//nop
			return cyc;
			break;

		case 0xed:	//nop
			return cyc;
			break;

		case 0xee:	//xor n
			_xor(_mmu->accessAt(_registers.pc++));
			cycle();
			break;

		case 0xef:	//rst
			rst(0x28);
			break;

		case 0xf0:	//ldh a,[ff+n]
			addr.addr = 0xFF00 + _mmu->accessAt(_registers.pc++);
			cycle();
			ld(&_registers.a, _mmu->accessAt(addr.addr));
			cycle();
			break;

		case 0xf1:	//pop af
			pop(&_registers.af);
			break;

		case 0xf2:	//ld a,[ff+c]
			addr.addr = 0xFF00 + _registers.c;
			ld(&_registers.a, _mmu->accessAt(addr.addr));
			cycle();
			break;

		case 0xf3:	//di
			di();
			break;

		case 0xf4:	//nop
			return cyc;
			break;

		case 0xf5:	//push af
			push(&_registers.af);
			break;

		case 0xf6:	//or n
			_or(_mmu->accessAt(_registers.pc++));
			cycle();
			break;

		case 0xf7:	//rst
			rst(0x30);
			break;

		case 0xf8:	//ldhl sp,n (signed)
			dis = _mmu->accessAt(_registers.pc++);
			ldhl(dis);
			break;

		case 0xf9:	//ld sp, hl
			cycle();
			ld(&_registers.sp, _registers.hl);
			break;

		case 0xfa:	//ld a, [nn]
			addr.n1 = _mmu->accessAt(_registers.pc++);
			cycle();
			addr.n2 = _mmu->accessAt(_registers.pc++);
			cycle();
			ld(&_registers.a, addr.addr);
			break;
/////go back and verify ld cycles the ppu properly
		case 0xfb:	//ei
			ei();
			break;

		case 0xfc:	//nop
			return cyc;
			break;

		case 0xfd:	//nop
			return cyc;
			break;

		case 0xfe:	//cp a,n
			cp(_mmu->accessAt(_registers.pc++));
			cycle();
			break;

		case 0xff:	//rst
			rst(0x38);
			break;

		default:
			std::cerr << "Unhandled opcode: 0x" << std::hex << +opcode << std::endl;
			exit(1);
			break;

	}
	_registers.f &= 0xF0;
	uint8_t inst_cyc_tab[][2] = {{4, 0}, {12, 0}, {8, 0}, {8, 0}, {4, 0}, {4, 0}, {8, 0}, {4, 0}, {20, 0}, {8, 0}, {8, 0}, {8, 0}, {4, 0}, {4, 0}, {8, 0}, {4, 0}, {4, 0}, {12, 0}, {8, 0}, {8, 0}, {4, 0}, {4, 0}, {8, 0}, {4, 0}, {12, 0}, {8, 0}, {8, 0}, {8, 0}, {4, 0}, {4, 0}, {8, 0}, {4, 0}, {12, 8}, {12, 0}, {8, 0}, {8, 0}, {4, 0}, {4, 0}, {8, 0}, {4, 0}, {12, 8}, {8, 0}, {8, 0}, {8, 0}, {4, 0}, {4, 0}, {8, 0}, {4, 0}, {12, 8}, {12, 0}, {8, 0}, {8, 0}, {12, 0}, {12, 0}, {12, 0}, {4, 0}, {12, 8}, {8, 0}, {8, 0}, {8, 0}, {4, 0}, {4, 0}, {8, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {8, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {8, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {8, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {8, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {8, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {8, 0}, {4, 0}, {8, 0}, {8, 0}, {8, 0}, {8, 0}, {8, 0}, {8, 0}, {4, 0}, {8, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {8, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {8, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {8, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {8, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {8, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {8, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {8, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {8, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {8, 0}, {4, 0}, {20, 8}, {12, 0}, {16, 12}, {16, 0}, {24, 12}, {16, 0}, {8, 0}, {16, 0}, {20, 8}, {16, 0}, {16, 12}, {4, 0}, {24, 12}, {24, 0}, {8, 0}, {16, 0}, {20, 8}, {12, 0}, {16, 12}, {0, 0}, {24, 12}, {16, 0}, {8, 0}, {16, 0}, {20, 8}, {16, 0}, {16, 12}, {0, 0}, {24, 12}, {0, 0}, {8, 0}, {16, 0}, {12, 0}, {12, 0}, {8, 0}, {0, 0}, {0, 0}, {16, 0}, {8, 0}, {16, 0}, {16, 0}, {4, 0}, {16, 0}, {0, 0}, {0, 0}, {0, 0}, {8, 0}, {16, 0}, {12, 0}, {12, 0}, {8, 0}, {4, 0}, {0, 0}, {16, 0}, {8, 0}, {16, 0}, {12, 0}, {8, 0}, {16, 0}, {4, 0}, {0, 0}, {0, 0}, {8, 0}, {16, 0}};
	if ((_inCycles != inst_cyc_tab[opcode][0]) && (_inCycles != inst_cyc_tab[opcode][1]))
	{
		printf("Error: %hhd != %hhd or 0x%hhd (pc: 0x%04hx\n opcode: 0x%02hhx)\n", _inCycles, inst_cyc_tab[opcode][0], inst_cyc_tab[opcode][1],_registers.pc, opcode);
		exit(1);
	}
	return cyc;
}
