##
## @file Makefile
## Betriebssysteme MyFind Makefile
## Beispiel 1
##
## @author Baliko Markus	    <ic15b001@technikum-wien.at>
## @author Haubner Alexander    <ic15b033@technikum-wien.at>
## @author Riedmann Michael     <ic15b054@technikum-wien.at>
## @date 2016/03/18
##
## @version $2.0 $
##

CC=gcc
GCCVERSION = $(shell gcc --version | grep ^gcc | sed 's/^.* //g')
CFLAGS=-Wall -Werror -Wextra -Wstrict-prototypes -pedantic -fno-common -O3 -g -std=gnu11
CP=cp
CD=cd
MV=mv
GREP=grep
DOXYGEN=doxygen

OBJECTS=main.o

#Annuminas Hotfix
ifeq "$(GCCVERSION)" "4.4.7-16)"
    CC=gcc52
endif


##
## ----------------------------------------------------------------- rules --
##

%.o: src/%.c
	$(CC) $(CFLAGS) -o $@ -c $<

##
## --------------------------------------------------------------- targets --
##

all: myfind

myfind: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	$(RM) *.o *~ myfind

distclean: clean
	$(RM) -r doc

doc: html

html:
	$(DOXYGEN) doxygen.dcf

test: myfind
	test/test-find.sh -q -t ./myfind -r test/bic-myfind

##
## ---------------------------------------------------------- dependencies --
##

##
## =================================================================== eof ==
##
