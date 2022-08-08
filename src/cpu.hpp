#ifndef CPU____
#define CPU____
#include "mmu.hpp"
#include "ppu.hpp"
//check endian

union address	{
	struct	{
		uint8_t n1;
		uint8_t n2;
	};
	uint16_t addr;
};

struct	registers {
	union {
		struct {
			uint8_t f;
			uint8_t a;
		};
		uint16_t af;
	};
	union {
		struct {
			uint8_t c;
			uint8_t b;
		};
		uint16_t bc;
	};
	union {
		struct {
			uint8_t e;
			uint8_t d;
		};
		uint16_t de;
	};
	union {
		struct {
			uint8_t l;
			uint8_t h;
		};
		uint16_t hl;
	};
	union {
		struct {
			uint8_t pcc;
			uint8_t pcp;
		};
		uint16_t	pc;
	};
	union {
		struct {
			uint8_t spp;
			uint8_t sps;
		};
		uint16_t	sp;
	};
};

enum bitflags {
	z = 1 << 7, n = 1 << 6, h = 1 << 5, cy = 1 << 4 };

class cpu {
	public:
		cpu(std::shared_ptr<mmu>, std::shared_ptr<ppu>);
		uint8_t		interrupt_check(void);
		void		reset();
		void		setInterrupt(uint8_t);
		uint8_t		opcode_parse(void);
		bool		imeCheck();
//		void		checkRom(void);
		uint64_t	cyc;

	private:
		std::shared_ptr<mmu>	_mmu;
		std::shared_ptr<ppu>	_ppu;
		struct	registers		_registers;
		uint8_t	_inCycles;
		bool	_halt;
		uint8_t	haltcheck;
		uint8_t	ime_set;
//		uint8_t	*_regtab(uint8_t);
//		uint16_t	*_pairtabddss(uint8_t);
//		uint16_t	*_pairtabqq(uint8_t);
		uint8_t	_ime;

		void	ld(uint8_t *reg, uint8_t val);
		void	ld(uint8_t *regd, uint16_t addr);
		void	ldd(uint8_t a);
		void	ldi(uint8_t a);
		void	ld(uint16_t *regp, uint16_t val);
		void	ld(union address addr, uint8_t val);
		void	ld(union address addr, union address val);
		void	ldhl(char n);
		void	push(uint16_t *regp);
		void	pop(uint16_t *regp);
		void	add(uint16_t *regp);
		void	add(char val);
		void	add(uint8_t val);
		void	adc(uint8_t val);
		void	sub(uint8_t val);
		void	sbc(uint8_t val);
		void	_or(uint8_t val);
		void	_xor(uint8_t val);
		void	_and(uint8_t val);
		void	cp(uint8_t val);
		void	inc(uint8_t *reg);
		void	inc(uint16_t *regp);
		void	dec(uint8_t *reg);
		void	dec(uint16_t *regp);
		void	swap(uint8_t *reg);
		void	daa(void);
		void	cpl(void);
		void	ccf(void);
		void	scf(void);
		void	halt(void);
		void	stop(void);
		uint8_t	di(void);
		uint8_t	ei(void);
		void	rlca(void);
		void	rla(void);
		void	rrca(void);
		void	rra(void);
		void	rlc(uint8_t *reg);
		void	rl(uint8_t *reg);
		void	rrc(uint8_t *reg);
		void	rr(uint8_t *reg);
		void	sla(uint8_t *reg);
		void	sra(uint8_t *reg);
		void	srl(uint8_t *reg);
		void	bit(uint8_t opcode);
		void	set(uint8_t opcode);
		void	res(uint8_t opcode);
		void	jp(uint16_t addr);
		void	jp(uint16_t addr, uint16_t ye);
		void	jr(char dis);
		void	jr(char dis, uint16_t ye);
		void	call(uint16_t addr);
		void	call(uint16_t addr, uint8_t ye);
		void	rst(uint16_t addr);
		void	ret(void);
		void	ret(uint8_t ye);
		void	reti(void);

		inline void	alu(uint8_t fun, uint8_t val)
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

		inline void	rot(uint8_t fun, uint8_t *val)
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

		inline void	acctab(uint8_t fun)
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
		inline void cycle()
		{
//			printf("cycle\n");
			cyc += 4;
			_mmu->timerInc(4);
			_ppu->cycle();
			_inCycles += 4;
		}
		friend class debuggerator;
	};

#endif
