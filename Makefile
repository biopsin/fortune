# Minimal Makefile ;)

VERSION = 0.1

fortune: fortune.c

dist: clean fortune.c Makefile
	cd .. && tar cf - fortune-$(VERSION) | bzip2 -9 > fortune-$(VERSION).tar.bz2

clean:
	rm -f fortune fortune.o core
