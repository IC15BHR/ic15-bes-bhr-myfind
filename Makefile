CC=gcc
GCCVERSION = $(shell gcc --version | grep ^gcc | sed 's/^.* //g')
CFLAGS=-Wall -pedantic -Werror -Wextra -Wstrict-prototypes -fno-common -g -O3 -std=gnu11
CP=cp
CD=cd
MV=mv
GREP=grep
DOXYGEN=doxygen

OBJECTS=main.o

ifeq "$(GCCVERSION)" "4.4.7"
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
	test/test-find.sh -t ./myfind -r test/bic-myfind

##
## ---------------------------------------------------------- dependencies --
##

##
## =================================================================== eof ==
##
