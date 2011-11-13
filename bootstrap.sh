#!/bin/sh

aclocal \
&& autoheader \
&& autoconf \
&& libtoolize \
&& automake --gnu --add-missing 
