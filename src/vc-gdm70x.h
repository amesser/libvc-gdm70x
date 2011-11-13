/*
vc-gdm70x.h

This file is part of libvc-gdm70x, a library to connect to Voltcraft GDM 70x
Multimeters via RS232.

Copyright (C) 2005 
Andreas Messer <andreas.messer@stud-mail.uni-wuerzburg.de>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef __VC_GDM70X__
#define __VC_GDM70X__

#include <termios.h>

#ifdef __cplusplus
extern "C" {
#endif

enum vc_unit { UNKNOWN,
	       VAC,
	       VDC,
	       AAC,
	       ADC,
	       OHM,
	       FARAD,
	       HERZ,
	       LOGIC,
	       DIODE,
	       TEMP_C,
	       TEMP_F,
	       RH,
	       PASCAL,
	       PSI };

enum vc_mult {
         MEGA='M',
	       KILO='k', 
	       NONE=' ',  
	       MILLI='m', 
	       MICRO='u', 
         NANO='n'
};

/* struct containing the received data from one channel */

struct vc_gdm70x_data {
  float value;
  enum vc_unit unit;
  enum vc_mult mult;
};

/* struct containing all import information of a GDM meter */

struct vc_gdm70x {
  signed int fd;
  struct termios oldtio; 

  int sync;

  struct vc_gdm70x_data data1; /* first channel */
  struct vc_gdm70x_data data2; /* second channel */

  unsigned char* image;

  int (* func_data) (struct vc_gdm70x* gdm_p, void* ptr);
  int (* func_image) (struct vc_gdm70x* gdm_p, void* ptr);

  void* func_data_ext;
  void* func_image_ext;

};


extern int vc_gdm70x_verbose;

/* vc_gdm70x_create: create a vc_gdm70x struct */
extern struct vc_gdm70x* vc_gdm70x_create();

/* vc_gdm70x_destroy: destroy a vc_gdm70x struct */
extern void vc_gdm70x_destroy(struct vc_gdm70x* gdm_p);

/* vc_gdm70x_setfunc_data: set the callback func for a record */
extern int vc_gdm70x_setfunc_data( struct vc_gdm70x* gdm_p, 
				   int (* func_data)(struct vc_gdm70x* gdm_p, void* ptr),
				   void* data_ptr);

/* vc_gdm70x_setfunc_image: set the callback func for a image */
extern int vc_gdm70x_setfunc_image( struct vc_gdm70x* gdm_p, 
				    int (* func_image) (struct vc_gdm70x* gdm_p, void* ptr),
				    void* image_ptr);

/* vc_gdm70x_open: open a tty for communication */
extern int  vc_gdm70x_open( struct vc_gdm70x* gdm_p, const char* device);

/* vc_gdm70x_close: close the tty */
extern void vc_gdm70x_close(struct vc_gdm70x* gdm_p);

/* vc_gdm70x_sync: syncronize with the GDM */
extern int vc_gdm70x_sync(struct vc_gdm70x* gdm_p);

/* vc_gdm70x_do: receive and evaluate data from the GDM */
extern int vc_gdm70x_do(struct vc_gdm70x* gdm_p, int skip);

#ifdef __cplusplus
}
#endif

#endif
