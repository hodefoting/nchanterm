CFILES  = $(wildcard *.c)
OBJS    = $(CFILES:.c=.o)
PROJNAME = nchanterm
LIBNAME = lib$(PROJNAME)
PREFIX = /usr/local
LIBEXT  = so
TARGETS = $(LIBNAME).a $(LIBNAME).$(LIBEXT)

CFLAGS += -I.. -I. 
CFLAGS += -O2 -g
CFLAGS += -Wall

all: $(TARGETS)

$(LIBNAME).a: $(OBJS)
	@echo "  AR" $@;$(AR) rcs $@ $(OBJS)
$(LIBNAME).$(LIBEXT): $(OBJS)
	@echo "  LD" $@;$(CC) -shared $(OBJS) -o $@ $(FLAGS) $(LIBS)
%.o: %.c *.h
	@echo "  CC" $<;$(CC) -c $(CFLAGS) $< -o $@

EXAMPLES_CFILES = $(wildcard examples/*.c)
EXAMPLES_BINS   = $(EXAMPLES_CFILES:.c=)

examples/%: examples/%.c $(LIBNAME).a
	@echo "CCLD" $@; $(CC) -I .. $(CFLAGS) $(LIBS) $< $(LIBNAME).a -lm -o $@

all: $(EXAMPLES_BINS)

clean:
	rm -f $(OBJS) $(TARGETS) $(BIN) $(EXAMPLES_BINS)

install: $(LIBNAME).$(LIBEXT)
	sudo install -t $(PREFIX)/lib $(LIBNAME).$(LIBEXT)
	sudo install -t $(PREFIX)/lib/pkgconfig $(PROJNAME).pc
	sudo install -d $(PREFIX)/include/$(PROJNAME)
	sudo install -t $(PREFIX)/include/$(PROJNAME) nchanterm.c nchanterm.h
