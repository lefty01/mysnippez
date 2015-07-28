
#CROSS=
CC=$(CROSS)gcc
LD=$(CROSS)ld
CFLAGS=-W -Wall -Wextra -Wpacked
CFLAGS += -Werror 
#CFLAGS += -save-temps
CFLAGS += -Wmissing-declarations -Wmissing-prototypes  -Wnested-externs
CFLAGS += -Wstrict-prototypes



SRCS = gpstraced.c timestamp.c laufz.c


APPS=$(basename $(SRCS))


all: $(APPS)



gpstraced: gpstraced.c gpstraced.h
	$(CC) $(CFLAGS) -g -O2 -o $@ $<



test: laufz
	@./laufz 10 1:00:00 | grep Pace > /tmp/laufz.test.1
	@diff /tmp/laufz.test.1  laufz.test.1 || exit 1

clean:
	$(RM) $(APPS) *.o *.i
	$(RM) /tmp/laufz.test.1

distclean: clean
	$(RM) *~
	$(APPS)

