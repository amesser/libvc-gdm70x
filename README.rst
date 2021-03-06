libvc-gdm70x Version 0.2
========================

libvc-gdm70x is a library to get the data from the Voltcraft 
Grafical Display Meters 703/704/705 multimeters. These are 
connected via RS232 to the computer. The library provides
routines for retreiving and evaluating the data from the 
meter. I included a small program which can be used as a 
data logger.

This library and program have been created by 
Andreas Messer <andi@bastelmap.de>. 
On errors or hints please contact me using the above 
email-address.

Compiling and Installing
------------------------

If using the source from github, you need to run the 
bootstrap.sh script at first. You need to have
the autotools (autoconf, automake and libtoolize)
installed::

  $ sh bootstrap.sh

You'll need a C-Compiler and the standard C-Headers to
compile the library and the programm. I think you have 
already unpacked the archive, if not, do so. Now move into
the directory libvc-gdm70x-<version>. First of all run
the configure-script with the options you think you
need::

	$ ./configure

As default all files get installed within /usr/local. You
may change this using the option::

	--prefix=<path>

when invoking the configure-script. For a more detailed 
description of the options of the configure-script please 
refer to the INSTALL file in the package-directory or run::

	$ ./configure --help

Now you can compile the source typing::

	$ make

If some error occures, please email me. Now you have to 
run as root::

	$ make install

Now, all files should have been installed. To confirm 
this, try to run::

	$ vc-gdm70x --version

You should get the version of the vc-gdm70x program, 
which can be used to retreive the data from a GDM. If you 
don't like the thing at all, type::

	$ make uninstall

to remove all files.

Using the library
-----------------

At the moment, i have no time to write a detailed description
of the library api. But its not that hard to understand. Have
a look at vc-gdm70x.c (source of a small tool) and to
vc-gdm70x.h in the src subdirectory to understand, how all 
works or ask me via email.

Applications
------------

I have written a small application, which can be used to 
obtain the measured data or an image from a GDM. Type::

	$ vc-gdm70x --help

to see how to use it. You can easily change the output format
too meet your needs. E.g. CSV output::

  $ vc-gdm70x -f "%S;%D1;%U1;%D2;%U2\n"

Besides that I'am currently working on a graphical application
for recording and displaying the data: https://github.com/amesser/mmgui 
