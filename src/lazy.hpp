#ifndef FUCKYOUASSHOLE
#define FUCKYOUASSHOLE
#include <stdlib.h>
#include <string.h>

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
			_start = (unsigned char *)malloc(size);
			_end = _start + size;
			_size = size;
			_e = 0xFF;
			for (unsigned i = 0; i < _size; i++)
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
		void	setSize(unsigned size)
		{
			if (_size)
				throw "error: size already set";
			_start = (unsigned char *)malloc(size);
			_end = _start + size;
			_size = size;
			_e = 0xFF;
			for (unsigned i = 0; i < _size; i++)
				*(_start + i) = 0xFF;
		}
		unsigned char getPix()
		{
			unsigned char pix = *_start;
			if (pix == _e)
				throw "Error: no valid pix";
			for (unsigned i = 0; i < _size - 1; i++)
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
			for (unsigned i = 0; i < _size; i++)
				*(_start + i) = 0xFF;
		}
		unsigned char &operator[](int index)
		{
			if (0 <= index && index < _size)
				return *(_start + index);
			return _e;
		}

	private:
		unsigned char *_start;
		unsigned char *_end;
		unsigned char _e;
		size_t _size;
};

#endif
