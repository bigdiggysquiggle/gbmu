#ifndef LAZ_E
#define LAZ_E
#include <stdlib.h>
#include <string.h>
#include <cstdint>

#include <cstdio>
//proof of concept for a less stupid container that would be
//used to load pixels into the main pixel array as they get
//generated

class laz_e {
	public:
		laz_e()
		{
			_start = NULL;
			_end = NULL;
			_size = 0;
			_e = 0xFF;
		}
		laz_e(size_t size)
		{
			_start = (uint8_t *)malloc(size);
			_end = _start + size;
			_size = size;
			_e = 0xFF;
			for (uint32_t i = 0; i < _size; i++)
				*(_start + i) = 0xFF;
		}
//		~laz_e()
//		{
//			if (_size)
//			{
//				free(_start);
//				_start = NULL;
//				_end = NULL;
//				_size = 0;
//			}
//		}
		void	setSize(uint32_t size)
		{
			if (_size)
				throw "error: size already set";
			_start = (uint8_t *)malloc(size);
			_end = _start + size;
			_size = size;
			_e = 0xFF;
			for (uint32_t i = 0; i < _size; i++)
				*(_start + i) = 0xFF;
		}
		uint8_t getPix()
		{
//			printf("contents: ");
//			for (uint32_t i = 0; i < _size; i++)
//				printf("%02x ", *(_start + i));
//			printf("\n");
			uint8_t pix = *_start;
			if (pix == _e)
				throw "Error: no valid pix";
			for (uint32_t i = 0; i < _size - 1; i++)
				*(_start + i) = *(_start + 1 + i);
			*(_start + _size - 1) = _e;
			return pix;
		}
		size_t getSize()
		{
			return _size;
		}
		void flush()
		{
			for (uint32_t i = 0; i < _size; i++)
				*(_start + i) = 0xFF;
		}
		uint8_t &operator[](int index)
		{
			size_t i = (size_t)index;
			if (i < _size)
				return *(_start + i);
			return _e;
		}

	private:
		uint8_t *_start;
		uint8_t *_end;
		uint8_t _e;
		size_t _size;
};

#endif
