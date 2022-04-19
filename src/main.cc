#include <unistd.h>
#include "debugger.hpp"
#include <exception>
#include <iostream>
#include <cstring>
#include <chrono>
#include <SDL2/SDL.h>

#define WIN_WIDTH 160
#define WIN_HEIGHT 144

//eventually will be used to generate a graphical
//interface which would then be able to control
//all aspects of the emulated hardware, including
//changing hardware as well as viewing and editting
//memory values

int main(int ac, char **av)
{
	if (ac == 1)
		printf("Error: no file given");
#ifdef DEBUG_PRINT_ON
	debuggerator _gb;
	try {
	_gb.setflags(ac - 2, av + 2);}
	catch (char const *e)
	{
		printf("%s\n", e);
		return (1);
	}
#else
	gb	_gb;
#endif
	try 
	{
		_gb.load_cart(av[1]);
	}
	catch (const char *msg)
	{
		std::cout << msg << std::endl;
		return 1;
	}
	SDL_Window *win;
//	SDL_Surface *screen;
	SDL_Texture *frame;
	SDL_Renderer *render;
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL fail\n");
		return 1;
	}
	atexit(SDL_Quit);
	win = SDL_CreateWindow("gbmu", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIN_WIDTH, WIN_HEIGHT, SDL_WINDOW_SHOWN);
//	screen = SDL_GetWindowSurface(win);
	if (!win)
	{
		printf("Window fail\n");
		return 1;
	}
	render = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
	frame = SDL_CreateTexture(render, SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STREAMING, 160, 144);
	SDL_Event e;
	bool	quit = false;
	SDL_PumpEvents();
	while (quit == false)
	{
		_gb.frame_advance();
		SDL_UpdateTexture(frame, NULL, _gb._ppu->pixels, (160 * 4));
		SDL_RenderCopy(render, frame, NULL, NULL);
		SDL_RenderPresent(render);
		if (SDL_PollEvent(&e) != 0 && (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)))
				quit = true;
	}
	SDL_DestroyRenderer(render);
	SDL_DestroyWindow(win);
	return 0;
}
