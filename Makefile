include config.mk
CFLAGS=-g -Wall -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED `pkg-config gtk+-2.0 --cflags` -std=c99 `pkg-config gtkglext-1.0 --cflags` -ggdb
LDFLAGS=`pkg-config gtk+-2.0 --libs` `pkg-config gtkglext-1.0 --libs` -lncurses

OBJS := $(patsubst %.c,%.o,$(wildcard *.c))

all: check $(OBJS)
	gcc $(OBJS) -o console $(LDFLAGS) -lusb -lpthread

.PHONY: check
check:
	if [ ! -e config.h ]; then ./configure; fi

.PHONY: dep
dep:
	$(CC) $(CCFLAGS) -MM *.c > .dep

.PHONY: clean
clean:
	rm -f $(OBJS) console

.PHONY: mrproper
mrproper: clean
	rm -f config.status config.log config.h tags .dep
	rm -fr autom4te.cache

.PHONY: install
install:
	echo "install it you"
	
-include .dep
