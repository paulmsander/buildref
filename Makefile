# Makefile to build the buildref tools

# Compiler and linker options
CFLAGS = -I/usr/local/include -g
LFLAGS = -L/usr/local/lib -g
#LIBS = -lchipset -lmsg
LIBS = -lchipset

# Installation info
INSTBASE = /usr/local
INSTBIN = $(INSTBASE)/bin
INSTLIB = $(INSTBASE)/lib
LOCALE = C
INSTMSG = $(INSTLIB)/locale/$(LOCALE)/LC_MESSAGES
MANSECT = 1
INSTMAN = $(INSTBASE)/man/man$(MANSECT)
#INSTMAN = /usr/catman/l_man/man$(MANSECT)

CC = gcc -ansi -pedantic

# Default rules
.SUFFIXES: .m4 .msg .xpg .$(MANSECT) .man

.m4.xpg:
	m4 $*.m4 > $@

.xpg.msg:
	gencat $@ $*.xpg

.o:
	$(CC) -o $@ $(LFLAGS) $@.o $(LIBS)

.man.$(MANSECT):
#	nroff -man $*.man | col > $@
	cp $*.man $@

# Build instructions
all: bldref.msg bldref paths.msg paths bldref.$(MANSECT) paths.$(MANSECT)

clean:
	rm -f *.o *.xpg *.msg bldref paths *.$(MANSECT) .bldref .bldref.old

bldref.o: bldref.c bldref.h
bldref.xpg: bldref.m4
bldref.msg: bldref.xpg
bldref.$(MANSECT): bldref.man

paths.o: paths.c paths.h
paths.xpg: paths.m4
paths.msg: paths.xpg
paths.$(MANSECT): paths.man

# Installation instructions
install: $(INSTBIN)/bldref $(INSTMSG)/bldref.msg $(INSTMAN)/bldref.$(MANSECT)
install: $(INSTBIN)/paths $(INSTMSG)/paths.msg $(INSTMAN)/paths.$(MANSECT)

$(INSTMSG)/bldref.msg: bldref.msg
	cp bldref.msg $@
	chmod 644 $@

$(INSTMSG)/paths.msg: paths.msg
	cp paths.msg $@
	chmod 644 $@

$(INSTBIN)/bldref: bldref
	cp bldref $@
	chmod 755 $@

$(INSTBIN)/paths: paths
	cp paths $@
	chmod 755 $@

$(INSTMAN)/bldref.$(MANSECT): bldref.$(MANSECT)
	cp bldref.$(MANSECT) $@
	chmod 644 $@

$(INSTMAN)/paths.$(MANSECT): paths.$(MANSECT)
	cp paths.$(MANSECT) $@
	chmod 644 $@

