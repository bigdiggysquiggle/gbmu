#include <stdio.h>
#include <memory.h>

#include "GBA.h"
#include "Globals.h"

#include "system.h"
#include "armdis.h"
#include "gb/GB.h"
#include "gb/gbGlobals.h"
#include "globals.h"

extern gbRegister PC;

FILE *trace=0;
bool map_trace=0;

bool *mapped=0;
bool *ram_mapped=0;

bool *rom_mapped=0;
bool *wram_board=0;
bool *wram_chip=0;
bool *sram_mapped=0;
extern bool hook_trace;

extern u32 pc_low, pc_high;

extern gbRegister AF;
extern gbRegister BC;
extern gbRegister DE;
extern gbRegister HL;
extern gbRegister SP;
extern gbRegister PC;

extern u8 register_STAT;
extern u8 register_IE;

extern unsigned int armNextPC;
extern bool armState;
extern int gbDis(char *buffer, u16 address);
extern FILE* fp_hook;
extern int armMode;

extern unsigned int rd_mode, wr_mode;
int pc_on = 0;

void Trace_Hook(char Format[], u32 addr, u32 value)
{
	int bank, offset;

	bank = armNextPC >> 16;
	offset = armMode ? (armNextPC - 2) : armNextPC-4;
	offset &= 0xffff;

	// hack: [%02X:%06X] --> [%04X:%04X]
	Format[3] = '4';
	Format[8] = '4';

	fprintf(fp_hook,Format, 
		bank,offset,value,addr);
	fprintf(fp_hook," ");

	// blanking status
	if(addr >= 0x6000000 && addr <= 0x6FFFFFF) {
		if( DISPSTAT & 1 ) fprintf(fp_hook, "V");
		if( DISPSTAT & 2 ) fprintf(fp_hook, "H");
		if( DISPCNT & 0x80 ) fprintf(fp_hook, "F");
		if( DISPCNT & 0xFF00 ) fprintf(fp_hook, " %02X", (DISPCNT&0xff00)>>8);
	}

	fprintf( fp_hook,"\n" );
}

void Trace_Hook_GB(char Format[], u32 addr, u32 value)
{
	if( ( rd_mode == 1 && Format[5] == 'R' ) ||
			( wr_mode == 1 && Format[5] == 'W' ) ) {
		fprintf(fp_hook,Format, 
			gbMemoryMap[PC.W>>12]-gbRom + (PC.W&0xfff),addr,value);
	}

	if( ( rd_mode == 0 && Format[5] == 'R' ) ||
			( wr_mode == 0 && Format[5] == 'W' ) ) {
		fprintf(trace,Format, 
			gbMemoryMap[PC.W>>12]-gbRom + (PC.W&0xfff),addr,value);
	}

	if( rd_mode == 2 && Format[5] == 'R' ) pc_on = 1;
	if( wr_mode == 2 && Format[5] == 'W' ) pc_on = 1;
}

void Trace_GBA_Msg(char String[])
{
	return;

	if(!trace || !map_trace)
		return;

	fprintf(trace,"%s",String);
}

void Trace_GBA()
{
	FILE *fp;
	char String [512];

	if(fp_hook && armNextPC>=(pc_low&(~0x03)) && armNextPC<=(pc_high|0x03)) {
		fp = fp_hook;
		goto pc_hook;
	}

	if(!map_trace)
		return;

	if(!trace) {
		trace=fopen("trace.log", "w");

		rom_mapped = new bool[2 * 0x100 * 0x10000];
		memset(rom_mapped,0,2 * 0x100 * 0x10000);

		wram_board = new bool[3 * 0x10000];
		memset(wram_board,0,3 * 0x10000);

		wram_chip = new bool[0x8000];
		memset(wram_chip,0,0x8000);

		sram_mapped = new bool[0x10000];
		memset(sram_mapped,0,0x10000);
	}

	fp = trace;

	if(armNextPC >= 0x02000000 && armNextPC <= 0x0203FFFF) {
		if(wram_board[armNextPC-0x2000000])
			return;
	}
	else if(armNextPC >= 0x03000000 && armNextPC <= 0x03007FFF) {
		if(wram_chip[armNextPC-0x3000000])
			return;
	}
	else if(armNextPC >= 0x08000000 && armNextPC <= 0x09FFFFFF) {
		if(rom_mapped[armNextPC-0x8000000])
			return;
	}
	else if(armNextPC >= 0x0E000000 && armNextPC <= 0x0E00FFFF) {
		if(sram_mapped[armNextPC-0xE000000])
			return;
	}
	else
		return;

pc_hook:
	if(armState)
   disArm(armNextPC, String, 3);
	else
   disThumb(armNextPC, String, 3);

	fprintf (fp, "%-56s", String);

	// registers
  for(int i = 0; i < 17; i++) {
    fprintf(fp, "R%02d=%08x ", i, reg[i].I);
  }

	// status flag
  fprintf(fp, "%c", (reg[16].I & 0x80000000) ? 'N' : 'n');
  fprintf(fp, "%c", (reg[16].I & 0x40000000) ? 'Z' : 'z');
  fprintf(fp, "%c", (reg[16].I & 0x20000000) ? 'C' : 'c');
  fprintf(fp, "%c", (reg[16].I & 0x10000000) ? 'V' : 'v');
  fprintf(fp, "%c", (reg[16].I & 0x80) ? 'I' : 'i');
  fprintf(fp, "%c", (reg[16].I & 0x40) ? 'F' : 'f');
  fprintf(fp, "%c", (reg[16].I & 0x20) ? 'T' : 't');
  fprintf(fp, " ");

	// blanking status
  if( DISPSTAT & 1 ) fprintf(fp, "V");
  if( DISPSTAT & 2 ) fprintf(fp, "H");
	if( DISPCNT & 0x80 ) fprintf(fp, "F");

	fprintf (fp, "\n");

	if(fp == fp_hook) return;

	if(armNextPC >= 0x02000000 && armNextPC <= 0x0203FFFF) {
		wram_board[armNextPC-0x2000000] = 1;
	}
	else if(armNextPC >= 0x03000000 && armNextPC <= 0x03007FFF) {
		wram_chip[armNextPC-0x3000000] = 1;
	}
	else if(armNextPC >= 0x08000000 && armNextPC <= 0x09FFFFFF) {
		rom_mapped[armNextPC-0x8000000] = 1;
	}
	else if(armNextPC >= 0x0E000000 && armNextPC <= 0x0E00FFFF) {
		sram_mapped[armNextPC-0xE000000] = 1;
	}
}

void Trace_GB_Msg(char String[])
{
	return;

	if(!trace || !map_trace)
		return;

	fprintf(trace,"%s",String);
}

void Trace_GB()
{
	int rom_address;
	FILE *fp;

	rom_address = gbMemoryMap[PC.W>>12]-gbRom + (PC.W&0xfff);

	if( hook_trace && ( pc_low == rom_address ) ) {
		pc_on = 1;
	}

	if( hook_trace && pc_on ) {
		if( hook_trace && ( pc_high == rom_address ) )
			pc_on = 0;

		fp = fp_hook;
		goto pc_hook;
	}

	if(!map_trace)
		return;

	if(!trace)
	{
		trace=fopen("trace.log", "w");
		mapped = new bool[0x4000 * 0x1000];
		memset(mapped,0,0x4000 * 0x1000);
		ram_mapped = new bool[0x8000];
		memset(ram_mapped,0,0x8000);
	}

	if( PC.W < 0x8000 && mapped[rom_address])
		return;
	if( ram_mapped[ PC.W-0x8000 ] )
		return;

	fp = trace;
	if( PC.W < 0x8000 ) mapped[rom_address] = 1;
	if( PC.W >= 0x8000 ) ram_mapped[PC.W - 0x8000] = 1;

pc_hook:
	char String [512], Buffer[512];

  gbDis(String, PC.W);

	sprintf( Buffer,"[%06x] %s",rom_address < 0 ? PC.W : rom_address, String);
	fprintf( fp, "%-35s", Buffer );

	fprintf( fp, " AF=%04x BC=%04x DE=%04x HL=%04x SP=%04x P1=%06x vc=%d ",
		AF.W, BC.W, DE.W, HL.W, SP.W,
		gbMemoryMap[0x04]-gbRom, register_LY );

	( register_STAT & 2 ) ? fprintf( fp, "B" ) : fprintf( fp, "b" );
	( register_IE ) ? fprintf( fp, "I" ) : fprintf( fp, "i" );
	
	fprintf( fp, "\n" );
}