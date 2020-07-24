/*
 * VisualBoyAdvanced - Nintendo Gameboy/GameboyAdvance (TM) emulator
 * Copyrigh(c) 1999-2002 Forgotten (vb@emuhq.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
 *
 * (c) Copyright 1996 - 2001 Gary Henderson (gary.henderson@ntlworld.com) and
 *                           Jerremy Koot (jkoot@snes9x.com)
 *
 * Super FX C emulator code 
 * (c) Copyright 1997 - 1999 Ivar (ivar@snes9x.com) and
 *                           Gary Henderson.
 * Super FX assembler emulator code (c) Copyright 1998 zsKnight and _Demo_.
 *
 * DSP1 emulator code (c) Copyright 1998 Ivar, _Demo_ and Gary Henderson.
 * C4 asm and some C emulation code (c) Copyright 2000 zsKnight and _Demo_.
 * C4 C code (c) Copyright 2001 Gary Henderson (gary.henderson@ntlworld.com).
 *
 * DOS port code contains the works of other authors. See headers in
 * individual files.
 *
 * Snes9x homepage: http://www.snes9x.com
 *
 * Permission to use, copy, modify and distribute Snes9x in both binary and
 * source form, for non-commercial purposes, is hereby granted without fee,
 * providing that this license information and copyright notice appear with
 * all copies and any derived work.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event shall the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Snes9x is freeware for PERSONAL USE only. Commercial users should
 * seek permission of the copyright holders first. Commercial use includes
 * charging money for Snes9x or software derived from Snes9x.
 *
 * The copyright holders request that bug fixes and improvements to the code
 * should be forwarded to them so everyone can benefit from the modifications
 * in future versions.
 *
 * Super NES and Super Nintendo Entertainment System are trademarks of
 * Nintendo Co., Limited and its subsidiary companies.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unzip.h"
#include "ips.h"

#define u8 unsigned char

static int type=0; // file type: *.ips, *.zip, *.rar, ...

// IPS patching adapted from Snes9x memmap.cpp file

// Read variable size MSB int from a file
static int utilReadInt(void *f, int nbytes)
{
  long v = 0;
  while (nbytes--) {
		int c;

		switch (type) {
		case 0:
			c = fgetc((FILE *) f);
			if (c == EOF) return -1;
			break;

		case 1:
			unzReadCurrentFile ((unzFile) f,&c,1);
			if (unzeof ((unzFile) f)) return -1;
			break;
		}

    v = (v << 8) | (c & 0xFF);
  }
  return (v);
}

static int IsIpsFile(const char *file)
{
  if(strlen(file) > 4) {
    char * p = strrchr(file,'.');

    if(p != NULL) {
      if(_stricmp(p, ".ips") == 0)
        return 1;
    }
  }

  return 0;  
}

static void IPSPatch(void *patch, u8 **r, int *s)
{
	char buffer[10];
  u8 *rom = *r;
  int size = *s;

	switch (type) {
	case 0: 
		if(fread (buffer, 1, 5, (FILE *) patch) != 5) return; 
		break;

	case 1:
		if(unzReadCurrentFile ((unzFile) patch,buffer,5) <= 0) return;
		break;
	}

	// check header
	if (strncmp (buffer, "PATCH", 5) != 0)
		return;
  
  for(;;) {
    int len;
    int c;
		int offset=0;

    offset = utilReadInt(patch, 3);
    if(offset == -1)
      goto err;

    // IPS end
    if (offset == 0x454f46)
      break;

    len = utilReadInt(patch, 2);
    if(len == -1)
      goto err;

    // if not zero, then it is a patch block
    if (len) {
      while(len--) {
        if(offset >= size) {
          rom = (u8 *)realloc(rom, (size<<1));
          *r = rom;
          *s = size = (size << 1);
        }
        c = utilReadInt(patch, 1);
        if(c == EOF) 
          goto err;
        rom[offset++] = (u8)c;
      }
    } else {
      // RLE block
      len = utilReadInt(patch, 2);
      if(len == -1)
        goto err;
      c = utilReadInt(patch, 1);
      
      if(c == EOF) 
        goto err;

      if((offset + len) >= size) {
        rom = (u8 *)realloc(rom, (size<<1));
        *r = rom;
        *s = size = (size << 1);        
      }
      
      while(len--) 
        rom[offset++] = (u8)c;
    }
  }

 err:
	return;
}

void ApplyIPS(const char *ips, u8 **image, int *size)
{
  FILE *patch = NULL;

	if (IsZipFile(ips)) {
	  char buffer[2048];
		int r;
   
		unzFile unz = unzOpen(ips);
		unz_file_info info;

		type=1;
   
		if(unz == NULL)
	    return;

		r = unzGoToFirstFile(unz);
    if(r != UNZ_OK) {
			unzClose(unz);
			return;
		}
    
		// search zip for all ips files
		while(1) {
			r = unzGetCurrentFileInfo(unz,
				                        &info,
					                      buffer,
						                    sizeof(buffer),
							                  NULL,
								                0,
									              NULL,
										            0);      
			if(r != UNZ_OK)
				break;

			// if not *.ips, skip to the next file
			if (!IsIpsFile (buffer))
				goto ipsnext;

			r = unzOpenCurrentFile(unz);
			if(r != UNZ_OK)
				break;

			// patch image
			IPSPatch (unz, image, size);

		ipsnext:
			// proceed to next file
			unzCloseCurrentFile(unz);
			r = unzGoToNextFile(unz);  
			if(r != UNZ_OK)
				break;
		}
		unzClose(unz);
	}
	else {
		type=0;
   
	  if(!(patch = fopen (ips, "rb")))
		  return;
		IPSPatch (patch, image, size);
		fclose (patch);
	}
}

int IsZipFile(const char *file)
{
  if(strlen(file) > 4) {
    char * p = strrchr(file,'.');

    if(p != NULL) {
      if(_stricmp(p, ".zip") == 0)
        return 1;
    }
  }

  return 0;
}

#if 0
int IsRarFile(const char *file)
{
  if(strlen(file) > 4) {
    char * p = strrchr(file,'.');

    if(p != NULL) {
      if(_stricmp(p, ".rar") == 0)
        return 1;
    }
  }

  return 0;  
}
#endif
