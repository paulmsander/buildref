$ This file contains the message definitions for the "paths" program.
$ The original source is "paths.m4", which is run through the m4 macro
$ preprocessor to produce "paths.xpg", which in turn is given to gencat
$ to make "paths.msg".

$ Also note that each change made here must also be reflected in messages.h
$ since that is where the message numbers are #defined for the paths program.
$ It would be nicer to generate both the message catalog and the messages.h
$ file from here.  Some day.

define(`usage',`Usage:\n\
paths [-a][-p prefix][-s suffix][-S separator] bldref class [path...]\n\
paths [-a][-p prefix][-s suffix][-S separator] -b build class [path...]\n\
paths [-l] bldref\n\
paths [-l] -b build
')

$set 1

1 usage

2 No bldref file was given\n\
usage

3 No search class was given\n\
usage

4 Unknown option:  %s\n\
usage

5 Could not open bldref file "%s"

6 Could not create list setup structure

7 Could not create buildref list

8 Could not add "%s" to buildref list

9 Length of build reference path "%s" is too long.\n\
It must be less than %d characters long.

10 Could not open search file "%s"

11 Length of search path "%s/%s" is too long.\n\
Perhaps there is a shorter path to %s that can be used.

12 Path missing for class "%s" in search file "%s"

13 Could not allocate memory to contain path "%s"

14 Path to file "%s" is "%s/%s",\n\
which is too long.  Perhaps there is a shorter path to the build that can\n\
be used.

15 System error accessing file "%s"

16 Could not find file "%s" in search path

17 Length of search path "%s" is too long.\n\
Perhaps there is a shorter path that can be used.

18 The -a and -l options are incompatible.

19 The -l and -p options are incompatible.

20 The -l and -s options are incompatible.

21 The -l and -S options are incompatible.

22 The -l option is incompatible with a search class.

