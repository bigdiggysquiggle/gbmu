#ifndef MEM
#define MEM
#include "print_debug.hpp"
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
//   and where info about hardware can be read
// 0xFF80 - 0xFFFE hram
// - slightly faster than other ram. 

#define DMG 1
#define GBC 2

class mmu {
	public:
		mmu(uint8_t);
		void pollInput(void);
		void loadCart(char *);
		void setINTS(void);
		void setVBLANK(void);
		void STATupdate(uint8_t);
//		~mmu();
		uint8_t	accessAt(uint16_t);
		uint8_t	PaccessAt(uint16_t);
		void			writeTo(uint16_t, uint8_t);
		void	timerInc(uint32_t cycles);
		inline void	dmaTransfer()
		{
			uint16_t byte = (640 - _oamtime) / 4;
//			writeTo(0xFE00 + byte, PaccessAt(((uint16_t)_IOReg[0x46] << 8) + byte));
			_oam[byte] = PaccessAt(((uint16_t)_IOReg[0x46] << 8) + byte);
			_oamtime = _oamtime - 4;
//			PRINT_DEBUG("_oamtime %u", _oamtime);
		}
		uint8_t	_cgb_mode;
		bool			vramWrite;
		uint16_t	_oamtime;
		//to implement: video access and write
		//				FF70 and FF4F cgb modes

	private:
		std::unique_ptr<cart>	_cart;
		std::vector<std::array<uint8_t, 0x2000>>	_vram;
		uint8_t	_wram0[0x1000];
		std::vector<std::array<uint8_t, 0x1000>>	_wram1;
		uint8_t	_oam[0xA0];
		uint8_t	_IOReg[0x80];
		uint8_t	_hram[0x80];
		uint8_t	_IE;
		uint32_t		_clock;
		uint32_t		_tac0;

		void	_IOwrite(uint16_t, uint8_t);
		friend class debuggerator;
};

#endif
