$ This file contains the message definitions for the "bldref" program.
$ The original source is "messages.m4", which is run through the m4 macro
$ preprocessor to produce "messages.xpg", which in turn is given to gencat
$ to make "bldref.msg".

$ Also note that each change made here must also be reflected in messages.h
$ since that is where the message numbers are #defined for the bldref program.
$ It would be nicer to generate both the message catalog and the messages.h
$ file from here.  Some day.

define(`usage',`Usage:\n\
bldref [-a] [-c] [-d directory] [-h] [-l] [-r] [-X] [build...]
')

define(`FileLine',`File:  %s\n\
Line:  %d\n
')

define(`PathFileLine',`Path:  %s\n\
FileLine
')

$set 1

1 usage

2 Unknown option:  %s\n\
usage

3 Operation of the bldref program is a no-op.  You must give either the\n\
-n option, or one or more build paths.\n\
usage

4 Cannot locate a Makefile in the present working directory.

5 Cannot invoke "make .top" to locate .bldref file.

6 The "TOP" macro in the Makefile is corrupt.  It specifies a path to a\n\
directory that is longer than %d characters.

7 Warning: The .bldref file cannot be opened for reading.\n\
File = %s

8 Cannot set up reference index

9 Cannot create reference index

10 Cannot create reference structure\n\
FileLine

11 Cannot create reference list node\n\
FileLine

12 Cannot duplicate path\n\
PathFileLine

13 Error inserting reference into index\n\
PathFileLine

14 Insertion into index revealed duplicate reference, but search failed\n\
PathFileLine

15 Warning:  Duplicate reference found\n\
PathFileLine

16 A command line argument gives a path that is too long.  It is %d\n\
characters long, but must be less than %d characters.\n\
Path:  %s

17 A build reference is too long.  It is %d characters long, but must be\n\
less than %d characters.\n\
PathFileLine

18 Command Line

19 Cannot unlink %s

20 Cannot access %s

21 Cannot create hard link\n\
Existing file: %s\n\
New file: %s

22 Cannot create file for writing\n\
File: %s

23 I/O error while writing to file\n\
File: %s

24 Cannot rename file\n\
Old name: %s\n\
New name: %s

