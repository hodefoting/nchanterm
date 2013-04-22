CFILES  = $(wildcard *.c)
OBJS    = $(CFILES:.c=.o)
PROJECT_NAME = nchanterm
LIBNAME = lib$(PROJECT_NAME)
PREFIX = /usr/local
LIBEXT  = so
TARGETS = $(LIBNAME).a $(LIBNAME).$(LIBEXT)

CFLAGS += -I.. -I. 
CFLAGS += -O2 -g
CFLAGS += -Wall
CFLAGS += -fPIC

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
	sudo install -t $(PREFIX)/lib/pkgconfig $(PROJECT_NAME).pc
	sudo install -d $(PREFIX)/include/$(PROJECT_NAME)
	sudo install -t $(PREFIX)/include/$(PROJECT_NAME) nchanterm.c nchanterm.h
