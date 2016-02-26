CC=gcc52
CFLAGS=-DDEBUG -Wall -pedantic -Werror -Wextra -Wstrict-prototypes -fno-common -g -O3 -std=gnu11
CP=cp
CD=cd
MV=mv
GREP=grep
DOXYGEN=doxygen

OBJECTS=src/main.o

EXCLUDE_PATTERN=footrulewidth

##
## ----------------------------------------------------------------- rules --
##

%.o: %.c
	$(CC) $(CFLAGS) -c $<

##
## --------------------------------------------------------------- targets --
##

all: myfind

hello: $(OBJECTS)
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