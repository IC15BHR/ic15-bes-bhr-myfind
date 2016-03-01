CC=gcc52
CFLAGS=-Wall -pedantic -Werror -Wextra -Wstrict-prototypes -fno-common -g -O3 -std=gnu11
CP=cp
CD=cd
MV=mv
GREP=grep
DOXYGEN=doxygen

OBJECTS=main.o

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
	test/test-find.sh -t myfind -r test/bic-myfind

##
## ---------------------------------------------------------- dependencies --
##

##
## =================================================================== eof ==
##
