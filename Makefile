CFLAGS = -O2 -Wall -std=c11 -pedantic
LIBS   = -lSDL2 -lSDL2_image -lSDL2_ttf

rcrun: rcrun.c Makefile
	$(CC) $(CFLAGS) -o $@ $(LIBS) $<

clean:
	rm -f rcrun

.PHONY: clean
