A gameboy/gameboy color emulator written in C++. I chose C++
as I felt that the features it offers would be the simplest
way to implement the different permutations of each cpu
instruction available on the gameboy/color's Sharp processor.
An added bonus I didn't realize until later was that using
C++ allowed me to utilize polymorphism to implement the
various types of MBC available in gameboy cartridges.

I initially set out on this project by designing my cpu. As I
wanted to remain as true and accurate to the gameboy as
possible, I designed a class that contained all of my
registers and set about implementing instructions first.
Initially I was having the cpu handle memory on its own, but
quickly realized that when I implemented video output I would
need a separate memory managing class to handle this for me.
Thus the mmu class was born.

My mmu holds all of the onboard memory as well as my cartridge
class, given that the gameboy memory map is designed such that
the memory in the cartridge is mapped alongside the system's
own memory. It also contains two different methods for memory
acces. One for general memory accesses and one specifically
for ppu memory accesses. I did this because the cpu isn't
allowed to access certain areas of memory when the ppu is
currently accessing them. As another piece of implementing
this functionality, I decided to use a shared pointer to
faclitate this shared access.

My cart class is where I really began to enjoy C++'s
polymorphism. I implemented a factory method for generating
the cart that would be stored in the mmu class to take as full
advantage of this as possible. Each ROM contains bytes that
designate the MBC type, ROM size, and RAM size. These make it
dead simple to determine what type of cart I need to create
and fill accordingly. This also allowed me to model MBCs as
accurately to how they actually function on physical hardware
as possible. The MBC serves as an intermediary between the
ROM and the cpu, containing registers that can be written to
in order to determine which ROM bank is currently being read
fROM and, if applicable, which RAM bank is currently being
used as well as whether or not the console is currently
allowed to write to said RAM. I really admire the way Nintendo
implemented this process. You write to an area in ROM, then
the MBC intercepts this write and sets its registers depending
on what area is written to and what the value in question is.

The ppu proved to be the most challenging aspect to implement
well. I don't have a strong graphics background but I do have
a strong math background so I feel like I had a little bit of
an advantage there. Even so there was a whole lot of trial and
error involved in making it work. Something I feel like isn't
explained particularly well is how many dots each mode takes.
Mode 3 can take anywhere from 168 to 291 dots depending on how
many sprites it finds in OAM that it needs to render on the
current scanline. This means that mode 0 takes 208 dots MINUS
how many extra dots mode 3 took to complete in order to
properly maintain the cycle count of 456 per scanline. These
456 dots translate to 114 cpu cycles, making a dot equal to
4 cycles.
"The Game Boy CPU and PPU run in parallel. The 4.2 MHz master
clock is also the dot clock. It's divided by 2 to form the
PPU's 2.1 MHz memory access clock, and divided by 4 to form a
multi-phase 1.05 MHz clock used by the CPU."
-- http://forums.nesdev.com/viewtopic.php?f=20&t=17754

If you've stumbled across this repo because you're building
your own emulator, hopefully some of my notes are of use to
you! I've included a list of links that I found useful while
developing this emulator. If you have further questions I can
be contacted on discord. Big D Squirrels#6472

https://cturt.github.io/cinoop.html
https://gbdev.io/pandocs/
https://gbdev.gg8.se/wiki
https://gb-archive.github.io/salvage/decoding_gbz80_opcodes/Decoding%20Gamboy%20Z80%20Opcodes.html
https://eldred.fr/gb-asm-tutorial/interrupts.html
http://www.codeslinger.co.uk/pages/projects/gameboy/dma.html
http://forums.nesdev.com/viewtopic.php?f=23&t=16612
http://forums.nesdev.com/viewtopic.php?f=20&t=17754&sid=7f6f26d4e925faff24b2528f6e52cc87

A surprisingly difficult piece of information to wrap my head
around was timing how the ppu reads in data and converts it
into an image. These two links held the info I needed to 
properly sort it out:

http://blog.kevtris.org/blogfiles/Nitty%20Gritty%20Gameboy%20VRAM%20Timing.txt
https://www.reddit.com/r/EmuDev/comments/59pawp/gb_mode3_sprite_timing/

Here's a collection of pdfs I used as reference during this
project:

http://marc.rawer.de/Gameboy/Docs/GBCPUman.pdf
--fantastic for understanding how instructions worked and
  getting their actual opcodes, though I found some of the
  information on flags to be a little bit off. The gbdev wiki
  has a fantastic page on what flags get set and reset with
  each instruction that helped clarifiy some of those issues.

https://gekkio.fi/files/gb-docs/gbctr.pdf
--this document was invaluable for getting an initial
  understanding of MBCs and how some of the different types
  function. Once I got that baseline understanding, the gbdev
  wiki was able to help me understand other MBC types even
  further.

https://github.com/AntonioND/giibiiadvance/blob/master/docs/TCAGBD.pdf
--the information presented in this document was especially
  thorough and useful for nailing down more of the fine
  details about how the gameboy functions at each level.

Further credits to blargg for this thorough testROM suite:
https://github.com/retrio/gb-test-ROMs

BGB for having an insanely powerful debugger

and Binji for the fantastic binjgb:
https://github.com/binji/binjgb

Binjgb's ability to output a stack trace made debugging my own
emulator significantly easier. I was able to output the stack
trace to a file, format my own stack trace indentically to
binjgb's, and diff the files to figure out exactly where my
emulation diverged fROM a proven emulator.
