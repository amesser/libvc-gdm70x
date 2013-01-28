/*
This program demonstrates the use of libvc-gdm70x, a library to connect to 
Voltcraft GDM 70x Multimeters via RS232.

Copyright (C) 2005-2013  Andreas Messer <andi@bastelmap.de>

This program is free software: you can redistribute it and/or modify
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
#include <unistd.h>
#include <getopt.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>

const struct option longopts [] = {
  { "device", required_argument, 0, 'd' },
  { "verbose",no_argument,0,'v'},
  { "format", required_argument,0,'f'},
  { "filename-format", required_argument,0,'F'},
  { "enable-image", no_argument,0,'i'},
  { "count",required_argument,0,'c'},
  { "version",no_argument,0,'V'},
  { "help",no_argument,0,'h'},
  {0,0,0,0}
};

const char default_device[] = "/dev/ttyS0";
const char default_print[] = "TIME: %S DATA1: %D1 %M1%U1 %T1; DATA2: %D2 %M2%U2 %T2\n";
const char default_file[] = "GDM70X-%Y%M%D-%h%m-%N.xpm";

static int verbose = 0;
static int record_count = 0;

static struct timespec ts_start;

int format_filename(char* filename, const char* format_string, const int count)
{
  int i=0,c;
  time_t current;
  struct tm current_time;

  assert(filename);
  assert(format_string);

  time(&current);
  localtime_r(&current, &current_time);

  if(verbose > 1)
    fputs("vc-gdm70x: format_filename: parsing string.\n",stderr);

 
  while( (c=*(format_string++)) != 0)
    {
      if( (c != '%') )
	filename[i++] = c;
      else
	{

	  switch(*(format_string++)) 
	    {
	    case 's': sprintf(filename + i,"%02i",current_time.tm_sec); i+= 2; break;
	    case 'm': sprintf(filename + i,"%02i",current_time.tm_min); i+= 2; break;
 	    case 'h': sprintf(filename + i,"%02i",current_time.tm_hour); i+= 2; break;
	    case 'D': sprintf(filename + i,"%02i",current_time.tm_mday); i+= 2; break;
	    case 'M': sprintf(filename + i,"%02i",current_time.tm_mon+1); i+= 2; break;
	    case 'Y': sprintf(filename + i,"%04i",current_time.tm_year+1900); i+= 4; break;
	    case 'y': sprintf(filename + i,"%02i",current_time.tm_year%100); i+= 2; break;

	    case 'N': sprintf(filename + i,"%04i",count); i+=4; break;
	    default:
	      fputs("vc-gdm70x: format_filename: error in filename string.\n",stderr);
	      filename[i] = 0;
	      return -1;
	    }
	} 
    }

  filename[i] = 0;
  return 0;

}

int print_values(struct vc_gdm70x* gdm_p, void* ptr) 
{
  int c;
  struct vc_gdm70x_data *data_p;
  assert(gdm_p);

  while( (c=*( (char*)ptr++)) != 0)
    {
      if( (c != '%') && (c != '\\') )
	fputc(c,stdout);
      else if( c == '%' )
	{
	  c = *((char*)ptr++);

	  if( c == 'D' || c == 'U' || c == 'M' || c == 'T')
	    {
	      switch(*((char*)ptr++)) 
		{
		case '1': data_p = &(gdm_p->data1); break;
		case '2': data_p = &(gdm_p->data2); break;
		default:
		  fputs("vc-gdm70x: error in formatstring.\n",stderr);
		  return -1;
		}
	    }

	  switch(c) 
	    {
	    case 'D': 
        if (data_p->mult != OVER)
          fprintf(stdout,"%.3f",data_p->value);
        else
          fputs("OVER",stdout);
        break;
	    case 'M': 
        if (data_p->mult != OVER)
          fputc(data_p->mult,stdout);
        break;
	    case 'U':
	      switch(data_p->unit)
		{
		case HERZ:   fputs("Hz",stdout); break;
		case VDC:
		case VAC:    fputs("V",stdout); break;
		case ADC:
		case AAC:    fputs("A",stdout); break;
		case OHM:    fputs("Ohm",stdout); break;
		case FARAD:  fputs("F",stdout); break;
		case LOGIC:  fputs("LOGIC",stdout); break;
		case DIODE:  fputs("DIODE",stdout); break;
		case TEMP_C: fputs("°C",stdout); break;
		case TEMP_F: fputs("°F",stdout); break;
		case RH:     fputs("RH",stdout); break;
		case PASCAL: fputs("Pa",stdout); break;
		case PSI:    fputs("Psi",stdout); break;

		default:    fputs("UNK",stdout); break;
		}
	      break;
	    case 'T':
	      switch(data_p->unit)
		{
		case VDC:
		case ADC: fputs("DC",stdout); break;
		case VAC:
		case AAC: fputs("AC",stdout); break;
		}
	      break;
	    case 'I': fprintf(stdout,"%i",record_count); break;
	    case 'C': fprintf(stdout,"%.3lf", (double) gdm_p->ts.tv_sec + 
	                                      (double) gdm_p->ts.tv_nsec * 1e-9);
		break;
	    case 'S': fprintf(stdout,"%.3lf", (double) (gdm_p->ts.tv_sec  - ts_start.tv_sec) + 
	                                      (double) (gdm_p->ts.tv_nsec - ts_start.tv_nsec) * 1e-9);
		break;
	    case '%': fputc('%',stdout); break;
	    default:
	      fputs("vc-gdm70x: error in formatstring.\n",stderr);
	      return -1;
	    }
	} 
      else if(c == '\\')
	{
	  switch(*((char*)ptr++))
	    {
	    case '\\': fputc('\\',stdout); break;
	    case 'n':  fputc('\n',stdout); break;
	    default:
	      fprintf(stderr,"vc-gdm70x: error in formatstring.\n");
	      return -1;
	      
	    }
	}
    }

  fflush(stdout);
  return 0;
}

int write_xpm(struct vc_gdm70x* gdm_p, void* ptr) {
  FILE * fp;
  int i;
  unsigned int n=0;
  char filename[512];

  assert(gdm_p);
  assert(ptr);

  if(verbose > 1)
    fputs("vc-gdm70x: write_xpm: creating filename.\n",stderr);

  do
    {
      format_filename(filename,(char*) ptr,n++);
      fp = fopen(filename,"r");
    }
  while(fp && n < 10000);

  if(fp)
    fclose(fp);

  if(n >= 10000)
    {
      fputs("vc-gdm70x: write_xpm: count overflow. Try removing some files.\n",stderr);
      return -1;
    }

  if(verbose)
    {
      fputs("vc-gdm70x: write_xpm: filename:'",stderr);
      fputs(filename,stderr);
      fputs("'.\n",stderr);
    }


  fp = fopen(filename,"w");

  if(fp) {
    fputs("/* XPM */\nstatic char* gdm70x[] = {\n \"128 64 2 1\",\n\"  c white\",\n\"O c black\"",fp);
    for(i = 0; i< 1024 * 8; i++) {
      if( (i%128) == 0)
	fputs(",\n\"",fp);
      if( gdm_p->image[i/8] & (0x80 >> (i%8)))
	fputc('O',fp);
      else
	fputc(' ',fp);
      
      if((i % 128) == 127)
	fputc('"',fp);
    }
    fputs("\n};",fp);
    fclose(fp);
  }
}


void print_help()
{
  printf("vc-gdm70x %s\n\n",VC_GDM70X_VERSION);
  puts("Usage: vc-gdm70x [options]\n");
  puts("Options: (default values are in brackets)");
  puts("  -h, --help                   displays this help and exit");
  puts("  -d, --device=DEVICE          RS232 device to which the GDM is connected"); 
  puts("                               [/dev/ttyS0]");
  puts("  -i, --enable-image           enables receiving of images");
  puts("  -c, --count=COUNT            number of records to fetch [0 (infinity)]");
  puts("      --filename-format=FORMAT format of the filename of the images");
  puts("                               [DATA1: %D1 %M1%U1 %T1; DATA2: %D2 %M2%U2 %T2\\n]");
  puts("  -f, --format=FORMAT          format of the output of the measured values");
  puts("                               [GDM70X-%Y%M%D-%h%m-%N.xpm]");
  puts("  -v, --verbose                makes output more noisy, repeating the switch");
  puts("                               increases level of noise");
  puts("  -V, --version                prints version info");
  puts("\nThe output can formated with the formatstring, which controls, what to be");
  puts("printed. There are special tokes, which will be replaced by the");
  puts("corresponding data.\n\nData tokens:");
  puts("  %D1, %D2 measured value of channel one or two as a 4 digit float with");
  puts("           decimal point");
  puts("  %M1, %M2 range of the measured value as a char 'M'=mega, 'k'=kilo,");
  puts("           ' '=none, 'm'=milli, 'n'=nano, p='pico'");
  puts("  %T1, %T2 AC or DC, or nothing");
  puts("  %U1, %U2 string showing what is measured e.g. V, A, F, Ohm...");
  puts("  %I       Number of Record from GDM");
  puts("  %C       Time the record was transmitted by GDM in seconds since epoch.");
  puts("  %I       Time the record was transmitted by GDM in seconds since program start.");
  puts("  %%       character '%'");
  puts("");
  puts("  \\n       newline");
  puts("  \\\\       character '\\'");
  puts("\nThe filename of the image file, created when a image is received");
  puts("can formated using a similar mechanism, but with other tokens.\n");
  puts("Filename tokens:");
  puts("  %s  actual time - seconds as 2 digit integer");
  puts("  %m  actual time - minutes as 2 digit integer");
  puts("  %h  actual time - hours as 2 digit integer");
  puts("  %D  actual date - day as 2 digit integer");
  puts("  %M  actual date - month as 2 digit integer");
  puts("  %Y  actual date - year as 4 digit integer");
  puts("  %y  actual date - year as 2 digit integer");
  puts("  %N  an 4 digit integer, which is counted up from zero until");
  puts("      an unused filename is found.");

}


int main(int argc, char** argv){
  struct vc_gdm70x* gdm_p;
  int c;
  int retval = 0;

  int enable_image = 0;
  const char* p_device = default_device;
  const char* p_print = default_print;
  const char* p_file = default_file;

  int record_max = 0;
  
  while( (c=getopt_long(argc,argv,":f:d:c:vihV",longopts,NULL)) != -1 )
    {
      switch(c) {
      case 'f':
	p_print = optarg;
	break;
      case 'd':
	p_device = optarg;
	break;
      case 'v':
	++verbose;++vc_gdm70x_verbose;
	break;
      case 'F':
	p_file = optarg;
	break;
      case 'i':
	enable_image = 1;
	break;
      case 'c':
	record_max = atoi(optarg);
	if(record_max < 0) {
	  retval = -1;
	  fprintf(stderr,"vc-gdm70x: count musst be greater or equal than 0.\n");
	}
	break;
      case ':':
	fprintf(stderr,"vc-gdm70x: option '-%c' requires an argument.\n",optopt);
	retval = -1;
	break;
      case 'V':
	printf("vc-gdm70x %s\n",VC_GDM70X_VERSION);
	exit(0);
	break;
      case 'h':
	print_help();
	exit(0);
      case '?':
      default:
	fprintf(stderr,"vc-gdm70x: unknown option '-%c'.\n",optopt);
	retval = -1;
	break;
      }
    }


  if(retval != 0)
    {
      fprintf(stderr,"vc-gdm70x: errors encountered, exiting.\n");
      exit(-1);
    }

  gdm_p = vc_gdm70x_create();

  if(!gdm_p) {
    fprintf(stderr,"vc-gdm70x: vc_gdm70x_create failed.\n");
    exit(-1);
  }

  vc_gdm70x_setfunc_data(gdm_p,print_values, (void*)p_print);

  if(enable_image)
    vc_gdm70x_setfunc_image(gdm_p,write_xpm,(void*)p_file);
  else
    vc_gdm70x_setfunc_image(gdm_p,0,0);

  if(verbose)
    fprintf(stderr,"vc-gdm70x: trying to open serial port.\n");

  if(vc_gdm70x_open(gdm_p, p_device)) {
    fprintf(stderr,"vc-gdm70x: vc_gdm70x_open failed.\n");
    vc_gdm70x_destroy(gdm_p);
    exit(-1);
  }

  if(verbose)
    fprintf(stderr,"vc-gdm70x: trying to sync with GDM.\n");

  if(vc_gdm70x_sync(gdm_p)) {
    fprintf(stderr,"vc_gdm70x: vc_gdm70x_sync failed.\n");
    vc_gdm70x_destroy(gdm_p);
    exit(-1);
  }

  if(verbose)
    fprintf(stderr,"vc-gdm70x: measuring.\n");

  clock_gettime(CLOCK_REALTIME,&ts_start);
  
  if(record_max == 0) {
    while(1)
      vc_gdm70x_do(gdm_p,0);
  } else {
    while(record_count++ < record_max) {
      vc_gdm70x_do(gdm_p,1);
    }
  }
  
  vc_gdm70x_destroy(gdm_p);

  if(verbose)
    fprintf(stderr,"vc-gdm70x: exiting successfully.\n");

  return 0;
}

