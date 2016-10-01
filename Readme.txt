PASCO 1.02
---------

Usage: pasco [options] <filename>
-d Undelete Activity Records
-t Field Delimiter (TAB by default)
-i Use ISO 8601 format for time stamp
-V Print version and then exit

Example Usage:

[kjones:pasco/bin]% ./pasco index.dat > index.txt


To recompile from source:

Enter the  "src" directory.  Type "make installwin" within Mingw32 
(or Cygwin, it can be, I'm not sure though) to make Pasco for Windows. 
Type "make install" to make Pasco for Unix.
The binaries will be located in the "bin" directory.

--------------
Originally written by
  Keith J. Jones
  keith.jones@foundstone.com
Modified by
  Yasuhito FUTATSUKI 
  <futatuki@yf.bsdclub.org>
--------------
$Id$
