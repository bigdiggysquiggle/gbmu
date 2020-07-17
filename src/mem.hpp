#ifndef MEM
#define MEM
#include "cart.hpp"
#include <SDL2/SDL.h>
#include <vector>

//0x0000 - 0x7fff ROM area
// - fixed bank 0, switchable bank 1
//0x8000 - 0x9fff vram area for tiles
// - 0x8000 - 0x97FF hold tiles themselves
// - - 0x10 byte tiles, 0x180 in 3 blocks of 0x80 
// - two tile maps, 0x9800-0x9BFF, 0x9C00-0x9FFF
// - - 32 x 32 bytes
// - can only access in modes 0, 1, 2
// 0xA000 - 0xBFFF sram
// - ram on cart. usually locked but can be unlocked
// - - usually returns 0xFF when locked
// - - unlock is MBC dependent
// 0xC000 - 0xDFFF wram
// - ram in console
// 0xE000 - 0xFDFF echo ram
// - echo of wram
// 0xFE00 - 0xFE9F oam ram (holds sprites/objects)
// - similar to vram in terms of locking
// 0xFEA0 - 0xFEFF "FEXX"
// - same locking behaviour as oam
// - behaviour is model and revision specific
// 0xFF00 - 0xFF7F IO
// - hardware mapped registers and such
// - this is where the actual hardware behaviour is configured
// 0xFF80 - 0xFFFE hram
// - slightly faster than other ram. 

class mmu {
	public:
		mmu();
		void pollInput(void);
		void loadCart(char *filename);
		void setINTS(void);
		void STATupdate(unsigned char);
//		~mmu();
		unsigned char	accessAt(unsigned short);
		unsigned char	PaccessAt(unsigned short);
		void			writeTo(unsigned short, unsigned char);
		void	timerInc(unsigned cycles);

		unsigned char	_cgb_mode;
		bool			vramWrite;
		unsigned short	_oamtime;
		//to implement: video access and write
		//				FF70 and FF4F cgb modes

	private:
		std::unique_ptr<cart>	_rom;
		std::vector<std::array<unsigned char, 0x2000>>	_vram;
		unsigned char	_wram0[0x1000];
		std::vector<std::array<unsigned char, 0x1000>>	_wram1;
		unsigned char	_oam[0xA0];
		unsigned char	_IOReg[0x80];
		unsigned char	_hram[0x80];
		unsigned char	_IE;
		unsigned		_clock;
		unsigned		_tac0;

		void	_IOwrite(unsigned short, unsigned char);
};

#endif
