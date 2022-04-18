#include "debugger.hpp"

//if the instructions that use a register pair as the address
//to write/read a value print the address in binjgb, set a
//third argument value which is the regpair number shifted
//over a certain number of bits
struct s_debugmsg	msgtab[] = {
	{"nop", 0},			//0x00
	{"ld bc,%hu", 2},
	{"ld [bc],a", 0},
	{"inc bc", 0},
	{"inc b", 0},
	{"dec b", 0},
	{"ld b,%hhu", 1},
	{"rlca", 0},
	{"ld $%04hx,sp", 2},
	{"add hl,bc", 0},
	{"ld a,[bc]", 0},
	{"dec bc", 0},
	{"inc c", 0},
	{"dec c", 0},
	{"ld c,%hhu", 1},
	{"rrca", 0},
	{"stop", 0},		//0x10
	{"ld de,%hu", 2},
	{"ld [de],a", 0},
	{"inc de", 0},
	{"inc d", 0},
	{"dec d", 0},
	{"ld d,%hhu", 1},
	{"rla", 0},
	{"jr %+hhd", 1},
	{"add hl,de", 0},
	{"ld a,[de]", 0},
	{"dec de", 0},
	{"inc e", 0},
	{"dec e", 0},
	{"ld e,%hhu", 1},
	{"rra", 0},
	{"jr nz,%+hhd", 1},	//0x20
	{"ld hl,%hu", 2},
	{"ld [hl+],a", 0},
	{"inc hl", 0},
	{"inc h", 0},
	{"dec h", 0},
	{"ld h,%hhu", 1},
	{"daa", 0},
	{"jr z,%+hhd", 1},
	{"add hl,hl", 0},
	{"ld a,[hl+]", 0},
	{"dec hl", 0},
	{"inc l", 0},
	{"dec l", 0},
	{"ld l,%hhu", 1},
	{"cpl", 0},
	{"jr nc,%+hhd", 1},	//0x30
	{"ld sp,%hu", 2},
	{"ld [hl-],a", 0},
	{"inc sp", 0},
	{"inc [hl]", 0},
	{"dec [hl]", 0},
	{"ld [hl],%hhu", 1},
	{"scf", 0},
	{"jr c,%+hhd", 1},
	{"add hl,sp", 0},
	{"ld a,[hl-]", 0},
	{"dec sp", 0},
	{"inc a", 0},
	{"dec a", 0},
	{"ld a,%hhu", 1},
	{"ccf", 0},
	{"ld b,b", 0},		//0x40
	{"ld b,c", 0},
	{"ld b,d", 0},
	{"ld b,e", 0},
	{"ld b,h", 0},
	{"ld b,l", 0},
	{"ld b,[hl]", 0},
	{"ld b,a", 0},
	{"ld c,b", 0},
	{"ld c,c", 0},
	{"ld c,d", 0},
	{"ld c,e", 0},
	{"ld c,h", 0},
	{"ld c,l", 0},
	{"ld c,[hl]", 0},
	{"ld c,a", 0},
	{"ld d,b", 0},		//0x50
	{"ld d,c", 0},
	{"ld d,d", 0},
	{"ld d,e", 0},
	{"ld d,h", 0},
	{"ld d,l", 0},
	{"ld d,[hl]", 0},
	{"ld d,a", 0},
	{"ld e,b", 0},
	{"ld e,c", 0},
	{"ld e,d", 0},
	{"ld e,e", 0},
	{"ld e,h", 0},
	{"ld e,l", 0},
	{"ld e,[hl]", 0},
	{"ld e,a", 0},
	{"ld h,b", 0},		//0x60
	{"ld h,c", 0},
	{"ld h,d", 0},
	{"ld h,e", 0},
	{"ld h,h", 0},
	{"ld h,l", 0},
	{"ld h,[hl]", 0},
	{"ld h,a", 0},
	{"ld l,b", 0},
	{"ld l,c", 0},
	{"ld l,", 0},
	{"ld l,e", 0},
	{"ld l,h", 0},
	{"ld l,l", 0},
	{"ld l,[hl]", 0},
	{"ld l,a", 0},
	{"ld [hl],b", 0},	//0x70
	{"ld [hl],c", 0},
	{"ld [hl],d", 0},
	{"ld [hl],e", 0},
	{"ld [hl],h", 0},
	{"ld [hl],l", 0},
	{"halt", 0},
	{"ld [hl],a", 0},
	{"ld a,b", 0},
	{"ld a,c", 0},
	{"ld a,d", 0},
	{"ld a,e", 0},
	{"ld a,h", 0},
	{"ld a,l", 0},
	{"ld a,[hl]", 0},
	{"ld a,a", 0},
	{"add a,b", 0},		//0x80
	{"add a,c", 0},
	{"add a,d", 0},
	{"add a,e", 0},
	{"add a,h", 0},
	{"add a,l", 0},
	{"add a,[hl]", 0},
	{"add a,a", 0},
	{"adc a,b", 0},
	{"adc a,c", 0},
	{"adc a,d", 0},
	{"adc a,e", 0},
	{"adc a,h", 0},
	{"adc a,l", 0},
	{"adc a,[hl]", 0},
	{"adc a,a", 0},
	{"sub a,b", 0},		//0x90
	{"sub a,c", 0},
	{"sub a,d", 0},
	{"sub a,e", 0},
	{"sub a,h", 0},
	{"sub a,l", 0},
	{"sub a,[hl]", 0},
	{"sub a,a", 0},
	{"sbc a,b", 0},
	{"sbc a,c", 0},
	{"sbc a,d", 0},
	{"sbc a,e", 0},
	{"sbc a,h", 0},
	{"sbc a,l", 0},
	{"sbc a,[hl]", 0},
	{"sbc a,a", 0},
	{"and a,b", 0},		//0xa0
	{"and a,c", 0},
	{"and a,d", 0},
	{"and a,e", 0},
	{"and a,h", 0},
	{"and a,l", 0},
	{"and a,[hl]", 0},
	{"and a,a", 0},
	{"xor a,b", 0},
	{"xor a,c", 0},
	{"xor a,d", 0},
	{"xor a,e", 0},
	{"xor a,h", 0},
	{"xor a,l", 0},
	{"xor a,[hl]", 0},
	{"xor a,a", 0},
	{"or a,b", 0},		//0xb0
	{"or a,c", 0},
	{"or a,d", 0},
	{"or a,e", 0},
	{"or a,h", 0},
	{"or a,l", 0},
	{"or a,[hl]", 0},
	{"or a,a", 0},
	{"cp a,b", 0},
	{"cp a,c", 0},
	{"cp a,d", 0},
	{"cp a,e", 0},
	{"cp a,h", 0},
	{"cp a,l", 0},
	{"cp a,[hl]", 0},
	{"cp a,a", 0},
	{"ret nz", 0},		//0xc0
	{"pop bc", 0},
	{"jp nz,$%04hx", 2},
	{"jp $%04hx", 2},
	{"call nz,$%04x", 2},
	{"push bc", 0},
	{"add a,%hhu", 1},
	{"rst 0x00", 0},
	{"ret z", 0},
	{"ret", 0},
	{"jp z,$%04x", 2},
	{0, 0}, //CB prefix reserved
	{"call z,$%04x", 2},
	{"call $%04x", 2},
	{"adc a,%hhu", 1},
	{"rst", 0},
	{"ret nc", 0},			//0xd0
	{"pop de", 0},
	{"jp nc,$%04hx", 2},
	{0, 0},					//0xD3 no op
	{"call nc,$%04hx", 2},
	{"push de", 0},
	{"sub a,%hhu", 1},
	{"rst", 0},
	{"ret c", 0},
	{"reti", 0},
	{"jp c,$%04hx", 2},
	{0, 0},					//0xDB no op
	{"call c,$%04x", 2},
	{0, 0},					//0xDD no op
	{0, 0},					//0xDE no op
	{"rst", 0},
	{"ldh [$ff%02x],a", 1},	//0xe0
	{"pop hl", 0},
	{"ld [$ff+c],a", 0},
	{0, 0},					//0xE3 no op
	{0, 0},					//0xE4 no op
	{"push hl", 0},
	{"and %hhu", 1},
	{"rst", 0},
	{"add sp,%+hhd", 1},
	{"jp [hl]", 0},
	{"ld [$%04x],a", 2},
	{0, 0},					//0xEB no op
	{0, 0},					//0xEC no op
	{0, 0},					//0xED no op
	{"xor %hhu", 1},
	{"rst", 0},
	{"ldh a,[$ff%02x]", 1},	//0xf0
	{"pop af", 0},
	{"ld a,[$ff+c]", 0},
	{"di", 0},
	{0, 0},					//0xF4 no op
	{"push af", 0},
	{"or %hhu", 1},
	{"rst", 0},
	{"ldhl sp,%+hhd", 1},
	{"ld sp,hl", 0},
	{"ld a,[$%04hx]", 2},
	{"ei", 0},
	{0, 0},					//0xFC no op
	{0, 0},					//0xFD no op
	{"cp a,%hhu", 1},
	{"rst", 0}
};

struct s_debugmsg cbtab[] ={
	{"rlc b", 0},       //0x00
	{"rlc c", 0},
	{"rlc d", 0},
	{"rlc e", 0},
	{"rlc h", 0},
	{"rlc l", 0},
	{"rlc [hl]", 0},
	{"rlc a", 0},
	{"rrc b", 0},
	{"rrc c", 0},
	{"rrc d", 0},
	{"rrc e", 0},
	{"rrc h", 0},
	{"rrc l", 0},
	{"rrc [hl]", 0},
	{"rrc a", 0},
	{"rl b", 0},        //0x10
	{"rl c", 0},
	{"rl d", 0},
	{"rl e", 0},
	{"rl h", 0},
	{"rl l", 0},
	{"rl [hl]", 0},
	{"rl a", 0},
	{"rr b", 0},
	{"rr c", 0},
	{"rr d", 0},
	{"rr e", 0},
	{"rr h", 0},
	{"rr l", 0},
	{"rr [hl]", 0},
	{"rr a", 0},
	{"sla b", 0},       //0x20
	{"sla c", 0},
	{"sla d", 0},
	{"sla e", 0},
	{"sla h", 0},
	{"sla l", 0},
	{"sla [hl]", 0},
	{"sla a", 0},
	{"sra b", 0},
	{"sra c", 0},
	{"sra d", 0},
	{"sra e", 0},
	{"sra h", 0},
	{"sra l", 0},
	{"sra [hl]", 0},
	{"sra a", 0},
	{"swap b", 0},      //0x30
	{"swap c", 0},
	{"swap d", 0},
	{"swap e", 0},
	{"swap h", 0},
	{"swap l", 0},
	{"swap [hl]", 0},
	{"swap a", 0},
	{"srl b", 0},
	{"srl c", 0},
	{"srl d", 0},
	{"srl e", 0},
	{"srl h", 0},
	{"srl l", 0},
	{"srl [hl]", 0},
	{"srl a", 0},
	{"bit 0,b", 0},     //0x40
	{"bit 0,c", 0},
	{"bit 0,d", 0},
	{"bit 0,e", 0},
	{"bit 0,h", 0},
	{"bit 0,l", 0},
	{"bit 0,[hl]", 0},
	{"bit 0,a", 0},
	{"bit 1,b", 0},
	{"bit 1,c", 0},
	{"bit 1,d", 0},
	{"bit 1,e", 0},
	{"bit 1,h", 0},
	{"bit 1,l", 0},
	{"bit 1,[hl]", 0},
	{"bit 1,a", 0},
	{"bit 2,b", 0},     //0x50
	{"bit 2,c", 0},
	{"bit 2,d", 0},
	{"bit 2,e", 0},
	{"bit 2,h", 0},
	{"bit 2,l", 0},
	{"bit 2,[hl]", 0},
	{"bit 2,a", 0},
	{"bit 3,b", 0},
	{"bit 3,c", 0},
	{"bit 3,d", 0},
	{"bit 3,e", 0},
	{"bit 3,h", 0},
	{"bit 3,l", 0},
	{"bit 3,[hl]", 0},
	{"bit 3,a", 0},
	{"bit 4,b", 0},     //0x60
	{"bit 4,c", 0},
	{"bit 4,d", 0},
	{"bit 4,e", 0},
	{"bit 4,h", 0},
	{"bit 4,l", 0},
	{"bit 4,[hl]", 0},
	{"bit 4,a", 0},
	{"bit 5,b", 0},
	{"bit 5,c", 0},
	{"bit 5,d", 0},
	{"bit 5,e", 0},
	{"bit 5,h", 0},
	{"bit 5,l", 0},
	{"bit 5,[hl]", 0},
	{"bit 5,a", 0},
	{"bit 6,b", 0},     //0x70
	{"bit 6,c", 0},
	{"bit 6,d", 0},
	{"bit 6,e", 0},
	{"bit 6,h", 0},
	{"bit 6,l", 0},
	{"bit 6,[hl]", 0},
	{"bit 6,a", 0},
	{"bit 7,b", 0},
	{"bit 7,c", 0},
	{"bit 7,d", 0},
	{"bit 7,e", 0},
	{"bit 7,h", 0},
	{"bit 7,l", 0},
	{"bit 7,[hl]", 0},
	{"bit 7,a", 0},
	{"res 0,b", 0},     //0x80
	{"res 0,c", 0},
	{"res 0,d", 0},
	{"res 0,e", 0},
	{"res 0,h", 0},
	{"res 0,l", 0},
	{"res 0,[hl]", 0},
	{"res 0,a", 0},
	{"res 1,b", 0},
	{"res 1,c", 0},
	{"res 1,d", 0},
	{"res 1,e", 0},
	{"res 1,h", 0},
	{"res 1,l", 0},
	{"res 1,[hl]", 0},
	{"res 1,a", 0},
	{"res 2,b", 0},     //0x90
	{"res 2,c", 0},
	{"res 2,d", 0},
	{"res 2,e", 0},
	{"res 2,h", 0},
	{"res 2,l", 0},
	{"res 2,[hl]", 0},
	{"res 2,a", 0},
	{"res 3,b", 0},
	{"res 3,c", 0},
	{"res 3,d", 0},
	{"res 3,e", 0},
	{"res 3,h", 0},
	{"res 3,l", 0},
	{"res 3,[hl]", 0},
	{"res 3,a", 0},
	{"res 4,b", 0},     //0xA0
	{"res 4,c", 0},
	{"res 4,d", 0},
	{"res 4,e", 0},
	{"res 4,h", 0},
	{"res 4,l", 0},
	{"res 4,[hl]", 0},
	{"res 4,a", 0},
	{"res 5,b", 0},
	{"res 5,c", 0},
	{"res 5,d", 0},
	{"res 5,e", 0},
	{"res 5,h", 0},
	{"res 5,l", 0},
	{"res 5,[hl]", 0},
	{"res 5,a", 0},
	{"res 6,b", 0},     //0xB0
	{"res 6,c", 0},
	{"res 6,d", 0},
	{"res 6,e", 0},
	{"res 6,h", 0},
	{"res 6,l", 0},
	{"res 6,[hl]", 0},
	{"res 6,a", 0},
	{"res 7,b", 0},
	{"res 7,c", 0},
	{"res 7,d", 0},
	{"res 7,e", 0},
	{"res 7,h", 0},
	{"res 7,l", 0},
	{"res 7,[hl]", 0},
	{"res 7,a", 0},
	{"set 0,b", 0},     //0xC0
	{"set 0,c", 0},
	{"set 0,d", 0},
	{"set 0,e", 0},
	{"set 0,h", 0},
	{"set 0,l", 0},
	{"set 0,[hl]", 0},
	{"set 0,a", 0},
	{"set 1,b", 0},
	{"set 1,c", 0},
	{"set 1,d", 0},
	{"set 1,e", 0},
	{"set 1,h", 0},
	{"set 1,l", 0},
	{"set 1,[hl]", 0},
	{"set 1,a", 0},
	{"set 2,b", 0},     //0xD0
	{"set 2,c", 0},
	{"set 2,d", 0},
	{"set 2,e", 0},
	{"set 2,h", 0},
	{"set 2,l", 0},
	{"set 2,[hl]", 0},
	{"set 2,a", 0},
	{"set 3,b", 0},
	{"set 3,c", 0},
	{"set 3,d", 0},
	{"set 3,e", 0},
	{"set 3,h", 0},
	{"set 3,l", 0},
	{"set 3,[hl]", 0},
	{"set 3,a", 0},
	{"set 4,b", 0},     //0xE0
	{"set 4,c", 0},
	{"set 4,d", 0},
	{"set 4,e", 0},
	{"set 4,h", 0},
	{"set 4,l", 0},
	{"set 4,[hl]", 0},
	{"set 4,a", 0},
	{"set 5,b", 0},
	{"set 5,c", 0},
	{"set 5,d", 0},
	{"set 5,e", 0},
	{"set 5,h", 0},
	{"set 5,l", 0},
	{"set 5,[hl]", 0},
	{"set 5,a", 0},
	{"set 6,b", 0},     //0xF0
	{"set 6,c", 0},
	{"set 6,d", 0},
	{"set 6,e", 0},
	{"set 6,h", 0},
	{"set 6,l", 0},
	{"set 6,[hl]", 0},
	{"set 6,a", 0},
	{"set 7,b", 0},
	{"set 7,c", 0},
	{"set 7,d", 0},
	{"set 7,e", 0},
	{"set 7,h", 0},
	{"set 7,l", 0},
	{"set 7,[hl]", 0},
	{"set 7,a", 0}
};

int			debuggerator::out_file;
unsigned	debuggerator::flags;
unsigned	debuggerator::format_type;
unsigned	debuggerator::output_data;
unsigned	debuggerator::i; 

struct sub_flags {
	const char *f_string;
	unsigned	f_val;
};

void	debuggerator::setData(int ac, char **av)
{
	static const struct sub_flags data_tab[] = {
		{"cpu", dataflags::cpu_instrs},
		{"mode", dataflags::ppu_mode},
		{"state", dataflags::ppu_state},
		{"vram", dataflags::ppu_vram},
		{"mmu", dataflags::mmu_access},
		{0, 0}
	};
	while (++i < ac && av[i][0] != '-')
	{
		for (int k = 0; data_tab[k].f_string; k++)
			if (!strcmp(data_tab[k].f_string, av[i]))
				output_data |= data_tab[k].f_val;
	}
	i--;
}

void	debuggerator::setFormat(int ac, char **av)
{
	static const struct sub_flags format_tab[] =
	{
		{"default", formatflags::default_output},
		{"binjgb", formatflags::binjgb},
		{0, 0}
	};
	i++;
	if (i == ac)
		throw "Error: missing output format";
	else
	{
		for (int k = 0; format_tab[k].f_string; k++)
			if (!strcmp(format_tab[k].f_string, av[i]))
			{
				format_type = format_tab[k].f_val;
				break;
			}
	}
}

void	debuggerator::setFile(int ac, char **av)
{
	i++;
	if (i != ac && av[i][0] != '-')
		out_file = open(av[i], O_WRONLY | O_CREAT | O_TRUNC, 0644);
}

void	debuggerator::setflags(int ac, char **av)
{
	const char *flagstr = "-o -f -d";
	static const struct s_flags flag_tab[] = {
		{flagset::output_file, &setFile},
		{flagset::output_format, &setFormat},
		{flagset::output_data, &setData},
	};
	for (i = 0; i < ac; i++)
	{
		const char *spot = strstr(flagstr, av[i]);
		unsigned x = (spot - flagstr) / 3;
		if (spot == NULL || x >= (sizeof(flag_tab) / sizeof(struct s_flags)))
			throw "Error: invalid flag";
		flags |= flag_tab[x].f_val;
		flag_tab[x].f_get(ac, av);
	}
	if (flags & flagset::output_file)
	{
		if (!out_file)
			out_file = open("debug_out.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	}
	else
		out_file = 1;
}

void	debuggerator::cpu_print()
{
//	if (_cpu->_registers.pc < 0x100)
//		return ;
	unsigned char instr = _mmu->PaccessAt(_cpu->_registers.pc);
	struct s_debugmsg db = (instr == 0xCB) ? cbtab[_mmu->PaccessAt(_cpu->_registers.pc+1)] : msgtab[instr];
	union address arg;
	if (format_type == (unsigned)formatflags::binjgb)
	{
		unsigned char _ppmode = _ppu->getMode();
		dprintf(out_file, "A:%02hhX F:%C%C%C%C BC:%04X DE:%04x HL:%04x SP:%04x PC:%04x (cy: %llu) ppu:%c%u ", _cpu->_registers.a, _cpu->_registers.f & 0x80 ? 'Z' : '-', _cpu->_registers.f & 0x40 ? 'N' : '-', _cpu->_registers.f & 0x20 ? 'H' : '-', _cpu->_registers.f & 0x10 ? 'C' : '-', _cpu->_registers.bc, _cpu->_registers.de, _cpu->_registers.hl, _cpu->_registers.sp, _cpu->_registers.pc, _cycles, _ppu->_off == false ? '+' : '-', _ppmode);
		if (_cpu->_registers.pc < 0x4000)
			dprintf(out_file, "|[00]");
		else if (_cpu->_registers.pc < 0x8000)
			dprintf(out_file, "|[%02X]", _mmu->_cart->getBank(_cpu->_registers.pc));
		else
			dprintf(out_file, "|[??]");
		dprintf(out_file, "0x%04x", _cpu->_registers.pc);
		dprintf(out_file, ": %02x", instr);
		if (db.args == 1)
		{
			char s[16];
			arg.n1 = _mmu->PaccessAt(_cpu->_registers.pc + 1);
			dprintf(out_file, " %02x     ", arg.n1);
			sprintf(s, db.str, arg.n1);
			dprintf(out_file, "%- 15s", s);
		}
		else if (db.args == 2)
		{
			char s[16];
			arg.n1 = _mmu->PaccessAt(_cpu->_registers.pc + 1);
			arg.n2 = _mmu->PaccessAt(_cpu->_registers.pc + 2);
			dprintf(out_file, " %02x %02x  ", arg.n1, arg.n2);
			sprintf(s, db.str, arg.addr);
			dprintf(out_file, "%- 15s", s);
		}
		else
			dprintf(out_file, "        %- 15s", db.str);
		dprintf(out_file, "\n");
	}
	else
	{
		char s[40];
		dprintf(out_file, "0x%02hhx ", instr);
		if (db.args == 1)
		{
			arg.n1 = _mmu->PaccessAt(_cpu->_registers.pc + 1);
			sprintf(s, db.str, arg.n1);
			dprintf(out_file, "%-14s\n", s);
		}
		else if (db.args == 2)
		{
			arg.n1 = _mmu->PaccessAt(_cpu->_registers.pc + 1);
			arg.n2 = _mmu->PaccessAt(_cpu->_registers.pc + 2);
			sprintf(s, db.str, arg.addr);
			dprintf(out_file, "%-14s\n", s);
		}
		else
			dprintf(out_file, "%-14s\n", db.str);
		dprintf(out_file, " 0x%04hx\n", _cpu->_registers.pc);
		dprintf(out_file, "\tregisters:\n\t");
		dprintf(out_file, "AF 0x%04hx BC 0x%04hx\n",_cpu->_registers.af, _cpu->_registers.bc);
		dprintf(out_file, "\tDE 0x%04hx HL 0x%04hx\n", _cpu->_registers.de, _cpu->_registers.hl);
		dprintf(out_file, "\tSP 0x%04hx PC 0x%04hx\n", _cpu->_registers.sp, _cpu->_registers.pc);
		dprintf(out_file, "\tLCDC 0x%02hhx STAT 0x%02hhx\n", _mmu->PaccessAt(0xFF40), _mmu->PaccessAt(0xFF41));
		dprintf(out_file, "\tLYC  0x%02hhx LY   0x%02hhx\n", _mmu->PaccessAt(0xFF45), _mmu->PaccessAt(0xFF44));
		dprintf(out_file, "\tIF 0x%02hhx IE 0x%02hhx IME %x\n\n\n", _mmu->PaccessAt(0xFF0F), _mmu->PaccessAt(0xFFFF), _cpu->_ime);
	}
}

void	debuggerator::debug_msg()
{
	if (out_file == 1)
		exit(0);
	if (output_data & dataflags::cpu_instrs)
		cpu_print();
	if (output_data & dataflags::ppu_mode)
		_ppu->getMode();
	if (output_data & dataflags::ppu_state)
		_ppu->getState();
}

void	debuggerator::frame_advance()
{
	unsigned framecount = 0;
	unsigned cyc;
	while (framecount < FRAME_TIME)
	{
		debug_msg();
		_mmu->pollInput();
		cyc = _cpu->opcode_parse();
//		PRINT_DEBUG("cyc = %u", cyc);
//		_mmu->timerInc(cyc);
		if (_cpu->imeCheck())
		{
			debug_msg();
			cyc += _cpu->opcode_parse();
		}
		if (_cpu->_registers.pc > 0x100 || (_cpu->_registers.pc < 0x100 && (_mmu->PaccessAt(0xFF50) & 1)))
			_cycles += cyc;
		framecount += cyc;
//		if (_cycles >= CPU_FREQ)
//			_cycles -= CPU_FREQ;
	}
//	for (unsigned i = 0; i < 23040; i++)
//		PRINT_DEBUG("0x%08X", _ppu->pixels[23040]);
}
