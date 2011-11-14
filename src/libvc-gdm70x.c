/*
This file is part of libvc-gdm70x, a library to connect to Voltcraft GDM 70x
Multimeters via RS232.

Copyright (C) 2005-2011  Andreas Messer <andi@bastelmap.de>

libvc-gdm70x is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "../config.h"
#include "vc-gdm70x.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <langinfo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

/* vc_gdm70x_parsevalue: parse a record, only for internal usage */
int vc_gdm70x_parsevalue(const char* str, struct vc_gdm70x_data* data_p);

int vc_gdm70x_read(struct vc_gdm70x* gdm_p, void* buf, int size);


int vc_gdm70x_verbose = 1;

struct vc_gdm70x* 
vc_gdm70x_create() 
{
  struct vc_gdm70x* ptr;

  ptr = malloc(sizeof(struct vc_gdm70x));

  if(!ptr) {
    if(vc_gdm70x_verbose)
      fputs("vc_gdm70x_create: malloc failed.\n",stderr);
    return 0;
  }

  memset(ptr,0,sizeof(struct vc_gdm70x));

  ptr->fd = -1;

  return ptr;
}

void 
vc_gdm70x_destroy(struct vc_gdm70x* gdm_p) 
{
  assert(gdm_p);

  if(gdm_p->fd > -1)
    vc_gdm70x_close(gdm_p);

  vc_gdm70x_setfunc_data(gdm_p,0,0);
  vc_gdm70x_setfunc_image(gdm_p,0,0);

  free(gdm_p);
}

int 
vc_gdm70x_setfunc_data( struct vc_gdm70x* gdm_p, 
			int (* func_data) (struct vc_gdm70x* gdm_p, void* ptr),
			    void* data_ptr) 
{
  assert(gdm_p);

  gdm_p->func_data = func_data;
  gdm_p->func_data_ext = data_ptr;

  return 0;
}

int 
vc_gdm70x_setfunc_image( struct vc_gdm70x* gdm_p, 
			 int (* func_image) (struct vc_gdm70x* gdm_p, void* ptr),
			 void* image_ptr) 
{
  assert(gdm_p);

  if(!gdm_p->image && func_image){
    gdm_p->image = malloc(1024);
    if(!gdm_p->image) {
      if(vc_gdm70x_verbose)
	fputs("vc_gdm70x_set_func: malloc failed.\n",stderr);
      gdm_p->func_image = 0;
      return -1;
    }
  } else if(gdm_p->image && !func_image) {
    free(gdm_p->image);
    gdm_p->image = 0;
  }

  gdm_p->func_image = func_image;
  gdm_p->func_image_ext = image_ptr;

  return 0;
}


int 
vc_gdm70x_open( struct vc_gdm70x* gdm_p, const char* device) 
{
  struct termios newtio;
  unsigned int data;

  assert(device); 
  assert(gdm_p);
  assert( gdm_p->fd < 0);

  gdm_p->fd = open(device, O_RDWR | O_NOCTTY );
  if(gdm_p->fd < 0) {
    if(vc_gdm70x_verbose)
      perror("vc_gdm70x_open: open failed");
    return -1;
  }

  if(tcgetattr(gdm_p->fd, &(gdm_p->oldtio))) {
    if(vc_gdm70x_verbose)
      perror("vc_gdm70x_open: tcgetattr failed");
    close(gdm_p->fd); gdm_p->fd = -1;
    return -1;
  }

  memcpy(&newtio,&(gdm_p->oldtio),sizeof(newtio));

  newtio.c_cflag &= ~(CSIZE | PARENB | CSTOPB | CRTSCTS);
  newtio.c_cflag |= (CS8 | CREAD | CLOCAL);
  newtio.c_iflag |= (IGNPAR | IGNBRK);
  newtio.c_iflag &= ~(INPCK | ISTRIP | IXON | IXOFF | IXANY);
  newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

  
  newtio.c_cc[VTIME] = 10;
  newtio.c_cc[VMIN] = 0;
  
  cfsetispeed(&newtio,B9600);
  cfsetospeed(&newtio,B9600);

  if( tcsetattr(gdm_p->fd,TCSAFLUSH, &newtio)) {
    if(vc_gdm70x_verbose)
      perror("vc_gdm70x_open: tcsetattr failed");
    close(gdm_p->fd); gdm_p->fd = -1;
    return -1;
  }

  if(ioctl(gdm_p->fd,TIOCMGET,&data) < 0 ) { /* or TIOCMBIC? */
    if(vc_gdm70x_verbose)
      perror("vc_gdm70x_open: ioctl failed");
    vc_gdm70x_close(gdm_p);
    return -1;
  }

  data &= ~(TIOCM_DTR & TIOCM_RTS); 

  if(ioctl(gdm_p->fd,TIOCMSET,&data) < 0) {/* or TIOCMBIC? */
    if(vc_gdm70x_verbose)
      perror("vc_gdm70x_open: ioctl failed");
    vc_gdm70x_close(gdm_p);
    return -1;
  }

  gdm_p->sync = 0;

  return 0;
}

void 
vc_gdm70x_close(struct vc_gdm70x* gdm_p) 
{
  assert(gdm_p);
  assert(gdm_p->fd >= 0);

  if( tcsetattr(gdm_p->fd,TCSAFLUSH, &(gdm_p->oldtio)))
    if(vc_gdm70x_verbose)
      perror("vc_gdm70x_close: tcsetattr failed");
    
  if(close(gdm_p->fd))
    if(vc_gdm70x_verbose)
      perror("vc_gdm70x_close: close failed");
  
  gdm_p->fd = -1;

  return;
}

int 
vc_gdm70x_sync(struct vc_gdm70x* gdm_p) 
{
  int i = 1026,bytes;
  char a;

  assert(gdm_p);
  assert(gdm_p->fd >= 0);

  gdm_p->sync = 0;

  while(i > 0){
    if( vc_gdm70x_read(gdm_p,&a,1) > 0) {
      if(a == 0x02)
	if(vc_gdm70x_read(gdm_p,&a,1) > 0) {

	  if(a == 'Z') 
	    i = 1025;
	  else
	    i = 24;

	  for(; i > 0; i-=bytes)
	    if( (bytes = vc_gdm70x_read(gdm_p,&a,1)) <= 0) {
		fputs("vc_gdm70x_sync: read failed.\n",stderr);
	      return -1;
	    }

	  if(a == 0x03) {
	    gdm_p->sync = 1;
	      if(vc_gdm70x_verbose > 2)
		fputs("vc_gdm70x_sync: synced.\n",stderr);
	    return 0;
	  }
	} 
      --i;
    } 
    else
      i=0;
  }

  return -1;
}


int 
vc_gdm70x_do(struct vc_gdm70x* gdm_p, int skip) 
{
  char buffer[26];
  signed int i,j,s,bytes;

  assert(gdm_p);
  assert(gdm_p->fd >= 0);

  if(gdm_p->sync == 0) {
    if(vc_gdm70x_verbose > 2)
      fputs("vc_gdm70x_do: gdm not synced, trying to sync.\n",stderr);
    if(vc_gdm70x_sync(gdm_p)) {
      if(vc_gdm70x_verbose > 1)
	fputs("vc_gdm70x_do: vc_gdm70x_sync failed.\n",stderr);
      return -1;
    }
  }

  do {
    memset(buffer,0,26);
    i = 0;
    i += vc_gdm70x_read(gdm_p, buffer,2);

    if(i < 2) {  
      if(vc_gdm70x_verbose > 1)
	fputs("vc_gdm70x_do: read failed.\n",stderr);
      return -1;
    }

    if( buffer[0] != 0x02 ) {
      if(vc_gdm70x_verbose > 1)
	fputs("vc_gdm70x_do: sync lost.\n",stderr);
      gdm_p->sync = 0;
      return -1;
    }
    
    if ( buffer[1] == 'Z' ) {
      if(gdm_p->image)
	memset(gdm_p->image,0,1024);

      j = 0;

      for(; i < 1027; i += bytes ) {
	if( (bytes = vc_gdm70x_read(gdm_p, buffer, ( 24 < i ) ? (24) : (i))) <= 0) {
	  if(vc_gdm70x_verbose > 1)
	    fputs("vc_gdm70x_do: read failed.\n",stderr);
	  return -1;
	}

	if(gdm_p->image)
	  for(s = 0; (s < bytes * 8) && (j < 8*1024); s++,j++)
	    gdm_p->image[( (i-2+s/8)%128 + ( ((i-2+s/8)/128)*8 + (s%8) )*128 )/8] |= ((buffer[s/8] & (0x01 << (s%8))) ? (0x80 >> ((i-2+s/8)%8)) : 0);
	  
	
      }
	
      
      if( *(buffer+bytes-1) != 0x03) {
	if(vc_gdm70x_verbose > 1)
	  fputs("vc_gdm70x_do: sync lost.\n",stderr);
	gdm_p->sync = 0;
	return -1;
      }
      
      if(gdm_p->func_image) {
	if( gdm_p->func_image(gdm_p,gdm_p->func_image_ext)) 
	  return -1; // return, if func_image returns != 0
	
      }	else
	if(vc_gdm70x_verbose > 2)
	  fputs("vc_gdm70x_do: picture dropped.\n",stderr);

    } 
    else {
      do{
	if( (bytes = vc_gdm70x_read(gdm_p,buffer + i, 26 - i)) <= 0) {
	  if(vc_gdm70x_verbose > 1)
	    fputs("vc_gdm70x_do: read failed.\n",stderr);
	  return -1;
	}
	i+= bytes;
      } while ( i < 26);

      if( buffer[25] != 0x03) {
	if(vc_gdm70x_verbose > 1)
	  fputs("vc_gdm70x_do: sync lost.\n",stderr);
	gdm_p->sync = 0;
	return -1;
      }


      if( vc_gdm70x_parsevalue(buffer+13,&(gdm_p->data2)))
	return -1;
      if( vc_gdm70x_parsevalue(buffer+1,&(gdm_p->data1)))
	return -1;

      if(gdm_p->func_data && !skip)
	if( gdm_p->func_data(gdm_p,gdm_p->func_data_ext))
	  return -1; // return, if func_data returns != 0
	
    }

    if( ioctl(gdm_p->fd,FIONREAD, &bytes) < 0) {
      if(vc_gdm70x_verbose)
	perror("vc_gdm70x_do: ioctl failed");
      return -1;
    }

  } while( (bytes >= 26) && (skip));

  if(gdm_p->func_data && skip)
    if( gdm_p->func_data(gdm_p,gdm_p->func_data_ext)) 
      return -1; // return, if func_data returns != 0
    
  return 0;
}


int 
vc_gdm70x_parsevalue(const char* str, struct vc_gdm70x_data* data_p) 
{
  int i;
  assert(data_p);
  assert(str);

  if(vc_gdm70x_verbose > 2)
    {
      fputs("vc_gdm70x_parsevalue: parse_string: '",stderr);
      fwrite(str,1,12,stderr);
      fputs("'.\n",stderr);
    }

  char *decimal_point = memchr(str+2,'.',6);

  if(decimal_point)
      *decimal_point = *nl_langinfo(RADIXCHAR);


  data_p->value = strtof(str+2,NULL);
  

  switch(*(str+8) )
    {
    case 'n': data_p->mult = NANO;  break;
    case 'm': data_p->mult = MILLI; break;
    case 'k': data_p->mult = KILO;  break;
    case 'M': data_p->mult = MEGA;  break; 
    case 'u': data_p->mult = MICRO; break;
    default:  data_p->mult = NONE;  break;
    }
  
  data_p->unit=UNKNOWN;

  switch(*str) 
    {
    case ' ': 
      if(strncmp(str+9,"Hz",2) == 0) data_p->unit=HERZ;
      if(strncmp(str+8,"@C",2) == 0) data_p->unit=TEMP_C; 
      if(strncmp(str+8,"@F",2) == 0) data_p->unit=TEMP_F; 
      if(strncmp(str+9,"Pa",2) == 0) data_p->unit=PASCAL; 
      if(strncmp(str+8,"Vdc",3) == 0) data_p->unit=VDC; 
      break;
    case 'A': 
    case 'C': data_p->unit = VAC; break;
    case 'B':
    case 'D': data_p->unit = VDC; break;
    case 'E': data_p->unit = OHM; break;
    case 'G': data_p->unit = DIODE; break;
    case 'H': data_p->unit = FARAD; break;
    case 'I': 
    case 'K': data_p->unit = AAC; break;
    case 'J': 
    case 'L': data_p->unit = ADC; break;
    case 'M': data_p->unit = LOGIC; break;
    case 'O':
      if(strncmp(str+8,"@C",2) == 0) data_p->unit=TEMP_C;
      if(strncmp(str+8,"@F",2) == 0) data_p->unit=TEMP_F;
      break;
    case 'P': data_p->unit = RH; break;
    case 'Q': data_p->unit = PSI; break;
    case 'R':
      if(strncmp(str+9,"Aac",3) == 0) data_p->unit=AAC;
      if(strncmp(str+9,"Adc",3) == 0) data_p->unit=ADC;
      break;
    default:
      if(vc_gdm70x_verbose)
	fprintf(stderr,"vc_gdm70x_parsevalue: unknown unit descriptor %c.\n",*(str));
      break;
    }

  return 0;
}

int
vc_gdm70x_read(struct vc_gdm70x* gdm_p, void* buf, const int size) 
{
  int i = 0;
  int bytes;

  assert(gdm_p);
  assert(gdm_p->fd >= 0);
  assert(buf);

  while(i < size) {
    if( (bytes = read(gdm_p->fd,(char*)buf+i,size - i)) == 0) {
      if(vc_gdm70x_verbose > 2)
	{
	  fputs("vc_gdm70x_read: read timeout.\n",stderr);
	}
      gdm_p->sync = 0;
      return -1;
    }
    i += bytes;
  }

  return i;
}
