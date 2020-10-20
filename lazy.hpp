#ifndef FUCKYOUASSHOLE
#define FUCKYOUASSHOLE
#include <stdlib.h>
#include <string.h>

class laz_e {
	public:
		laz_e()
		{
			_start = NULL;
			_end = NULL;
			_Aiter = NULL;
			_Giter = NULL;
			_size = 0;
			_e = 0xFF;
		}
		laz_e(size_t size)
		{
			_start = (unsigned char *)malloc(size);
			_end = _start + size;
			_Aiter = _start;
			_Giter = _start;
			_size = size;
		}
		~laz_e()
		{
			if (_size)
			{
				free(_start);
				_end = NULL;
				_Aiter = NULL;
				_Giter = NULL;
				_size = 0;
			}
		}
		unsigned char &getPix()
		{
			if (_Giter != _end)
				return (*(_Giter++));
			return _e;
		}
		void addPix(unsigned char pix)
		{
			if (_Aiter != _end && 0 <= pix && pix <= 3)
				*(_Aiter++) = pix;
		}
		void resize(size_t size)
		{
			unsigned char *tmp = (unsigned char *)malloc(size);
			memcpy(tmp, _start, size > _size ? _size : size);
			free(_start);
			_Aiter = (_Aiter - _start > size) ? tmp + size : tmp + (_Aiter - _start);
			_Giter = (_Giter - _start > size) ? tmp + size : tmp + (_Giter - _start);
			_start = tmp;
			_end = _start + size;
			_size = size;
		}
		size_t getSize()
		{
			return _size;
		}
		unsigned char &operator[](int index)
		{
			if (0 <= index && index < _size)
				return _start[index];
			return _e;
		}

	private:
		unsigned char *_start;
		unsigned char *_end;
		unsigned char *_Aiter;
		unsigned char *_Giter;
		unsigned char _e;
		size_t _size;
};

#endif
