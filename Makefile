IDIR=$(shell pwd)
CC=gcc
CFLAGS=-I$(IDIR)

ODIR=obj

LIBS=-lwiringPi

_DEPS = XSPI.h XNAND.h unpack.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = main.o XSPI.o XNAND.o unpack.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS)

xbox-flasher: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ xbox-flasher 
