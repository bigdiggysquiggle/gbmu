#include <unistd.h>
#include "cpu.hpp"
#include "cart.hpp"
#include "mem.hpp"
#include "ppu.hpp"
#include <exception>
#include <iostream>
#include <cstring>
#include <chrono>
#include <SDL2/SDL.h>

#define WIN_WIDTH 160
#define WIN_HEIGHT 144

int main(int ac, char **av)
{
	if (ac != 2 && ac != 3)
		return 0;
	if (ac == 3 && strcmp("-d", av[2]))
		return 1;
	auto memunit = std::make_shared<mmu>();
	cpu	processor(memunit);
	ppu graphics(memunit);
	processor.debug = ac == 3 ? true : false;
	try 
	{
		processor._mmu->loadCart(av[1]);
//		processor.checkRom();
	}
	catch (const char *msg)
	{
		std::cout << msg << std::endl;
		return 1;
	}
	SDL_Window *win;
	SDL_Surface *screen;
	SDL_Texture *frame;
	SDL_Renderer *render;
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL fail\n");
		return 1;
	}
	atexit(SDL_Quit);
	win = SDL_CreateWindow("gbmu", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIN_WIDTH, WIN_HEIGHT, SDL_WINDOW_SHOWN);
	screen = SDL_GetWindowSurface(win);
	if (!win)
	{
		printf("Window fail\n");
		return 1;
	}
	render = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
	frame = SDL_CreateTexture(render, SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STREAMING, 160, 144);
	unsigned	cycles = 1;
	SDL_Event e;
	bool	quit = false;
	while (quit == false)
	{
		memunit->timerInc(cycles);
		processor.opcode_parse();
		if (graphics.frameRender())
		{
			SDL_UpdateTexture(frame, NULL, graphics.pixels, (160 * 4));
			SDL_RenderCopy(render, frame, NULL, NULL);
			SDL_RenderPresent(render);
		}
		cycles = (cycles == 4194304) ? 1 : cycles + 1;
		if (SDL_PollEvent(&e) && e.type == SDL_QUIT)
			quit = true;
	}
	SDL_DestroyRenderer(render);
	SDL_DestroyWindow(win);
	return 0;
}
