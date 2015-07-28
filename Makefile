
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


clean:
	$(RM) $(APPS) *.o *.i

distclean: clean
	$(RM) *~
	$(APPS)

