CC=gcc
CFLAGS=-DDEBUG -Wall -pedantic -Werror -Wextra -Wstrict-prototypes -fno-common -g -O3 -std=gnu11
CP=cp
CD=cd
MV=mv
GREP=grep
DOXYGEN=doxygen

OBJECTS=output/main.o

##
## ----------------------------------------------------------------- rules --
##

output/%.o: src/%.c
	$(CC) $(CFLAGS) -o $@ -c $<

##
## --------------------------------------------------------------- targets --
##

all: output/myfind

output/myfind: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	$(RM) output/*/*.o output/*/*~ output/*/myfind

distclean: clean
	$(RM) -r doc

doc: html

html:
	$(DOXYGEN) doxygen.dcf

##
## ---------------------------------------------------------- dependencies --
##

##
## =================================================================== eof ==
##