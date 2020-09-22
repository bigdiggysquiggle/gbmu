#ifndef CPU____
#define CPU____
#include "mmu.hpp"
#include "ppu.hpp"
//check endian

union address	{
	struct	{
		unsigned char n1;
		unsigned char n2;
	};
	unsigned short addr;
};

struct	registers {
	union {
		struct {
			unsigned char f;
			unsigned char a;
		};
		unsigned short af;
	};
	union {
		struct {
			unsigned char c;
			unsigned char b;
		};
		unsigned short bc;
	};
	union {
		struct {
			unsigned char e;
			unsigned char d;
		};
		unsigned short de;
	};
	union {
		struct {
			unsigned char l;
			unsigned char h;
		};
		unsigned short hl;
	};
	union {
		struct {
			unsigned char pcc;
			unsigned char pcp;
		};
		unsigned short	pc;
	};
	union {
		struct {
			unsigned char spp;
			unsigned char sps;
		};
		unsigned short	sp;
	};
};

enum bitflags {
	z = 1 << 7, n = 1 << 6, h = 1 << 5, cy = 1 << 4 };

class cpu {
	public:
		cpu(std::shared_ptr<mmu>, std::shared_ptr<ppu>);
		std::shared_ptr<mmu>	_mmu;
		std::shared_ptr<ppu>	_ppu;
		unsigned char	interrupt_check(void);
		void			reset();
		void			setInterrupt(unsigned char);
		unsigned char	opcode_parse(unsigned char);
		unsigned char	opcode_parse(void);
		bool			debug;
//		void		checkRom(void);

	private:
		struct registers		_registers;
		unsigned char	_inCycles;
		bool			_halt;
//		unsigned char	*_regtab(unsigned char);
//		unsigned short	*_pairtabddss(unsigned char);
//		unsigned short	*_pairtabqq(unsigned char);
		unsigned char			_ime;

		void			ld(unsigned char *reg, unsigned char val);
		void			ld(unsigned char *regd, unsigned short addr);
		void			ldd(unsigned char a);
		void			ldi(unsigned char a);
		void			ld(unsigned short *regp, unsigned short val);
		void			ld(union address addr, unsigned char val);
		void			ld(union address addr, union address val);
		void			ldhl(char n);
		void			push(unsigned short *regp);
		void			pop(unsigned short *regp);
		void			add(unsigned short *regp);
		void			add(char val);
		void			add(unsigned char val);
		void			adc(unsigned char val);
		void			sub(unsigned char val);
		void			sbc(unsigned char val);
		void			_or(unsigned char val);
		void			_xor(unsigned char val);
		void			_and(unsigned char val);
		void			cp(unsigned char val);
		void			inc(unsigned char *reg);
		void			inc(unsigned short *regp);
		void			dec(unsigned char *reg);
		void			dec(unsigned short *regp);
		void			swap(unsigned char *reg);
		void			daa(void);
		void			cpl(void);
		void			ccf(void);
		void			scf(void);
		void			halt(void);
		void			stop(void);
		unsigned char	di(void);
		unsigned char	ei(void);
		void			rlca(void);
		void			rla(void);
		void			rrca(void);
		void			rra(void);
		void			rlc(unsigned char *reg);
		void			rl(unsigned char *reg);
		void			rrc(unsigned char *reg);
		void			rr(unsigned char *reg);
		void			sla(unsigned char *reg);
		void			sra(unsigned char *reg);
		void			srl(unsigned char *reg);
		void			bit(unsigned char opcode);
		void			set(unsigned char opcode);
		void			res(unsigned char opcode);
		void			jp(unsigned short addr);
		void			jp(unsigned short addr, unsigned short ye);
		void			jr(char dis);
		void			jr(char dis, unsigned short ye);
		void			call(unsigned short addr);
		void			call(unsigned short addr, unsigned char ye);
		void			rst(unsigned short addr);
		void			ret(void);
		void			ret(unsigned char ye);
		void			reti(void);

		inline void	alu(unsigned char fun, unsigned char val)
		{
			switch(fun)
			{
				case 0:
					add(val);
					break;
				case 1:
					adc(val);
					break;
				case 2:
					sub(val);
					break;
				case 3:
					sbc(val);
						break;
				case 4:
					_and(val);
					break;
				case 5:
					_xor(val);
					break;
				case 6:
					_or(val);
					break;
				case 7:
					cp(val);
					break;
			}
		}

		inline void	rot(unsigned char fun, unsigned char *val)
		{
			switch(fun)
			{
				case 0:
					rlc(val);
					break;
				case 1:
					rrc(val);
					break;
				case 2:
					rl(val);
					break;
				case 3:
					rr(val);
					break;
				case 4:
					sla(val);
					break;
				case 5:
					sra(val);
					break;
				case 6:
					swap(val);
					break;
				case 7:
					srl(val);
					break;
			}
		}

		inline void	acctab(unsigned char fun)
		{
			switch(fun)
			{
					case 0:
					rlca();
					break;
				case 1:
					rrca();
					break;
				case 2:
					rla();
					break;
				case 3:
						rra();
				break;
				case 4:
					daa();
					break;
				case 5:
					cpl();
					break;
				case 6:
					scf();
					break;
				case 7:
					ccf();
					break;
			}
		}
	};

#endif
