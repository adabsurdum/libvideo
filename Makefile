
GRAPHICS_FILE_FORMAT_DIR=../libgraphicsff
GRAPHICS_FILE_FORMAT_LIB=graphicsff

STATICLIB=libvideo.a
BUILD_ARCH=$(shell uname -m)

ifdef DEBUG
CFLAGS+=-g
CPPFLAGS+=-D_DEBUG
else
CFLAGS+=-O3
CFLAGS+=-DNDEBUG
endif

CFLAGS+=-Wall -std=c99
# c99 necessary to use the inline keyword.
# But note from https://gcc.gnu.org/onlinedocs/gcc/Inline.html#Inline:
#  GCC does not inline any functions when not optimizing unless you specify
#  the ‘always_inline’ attribute for the function, like this: 
#  // Prototype.
#  inline void foo (const char) __attribute__((always_inline));

CPPFLAGS+=-DHAVE_SINGLETON_DEVICE
# The resulting executable will only monitor one video device per process.

LDFLAGS=-L$(GRAPHICS_FILE_FORMAT_DIR)
LDLIBS=-l$(GRAPHICS_FILE_FORMAT_LIB)

############################################################################

OBJECTS=video.o \
	fourcc.o \
	firstdev.o \
	yuyv.o

############################################################################
# Rules

all : $(STATICLIB) yuyv2img

# Following object dependencies use GNU make's implicit rules.

# Core modules.

video.o  : video.h vidfmt.h vidfrm.h fourcc.h

# Helper/accessory modules

fourcc.o   : fourcc.h
firstdev.o :
convyuyv.o : convyuyv.c
	$(CC) -c -o $@ $(CFLAGS) -I../libgraphicsff $<

$(STATICLIB) : $(OBJECTS)
	$(AR) rcs $@ $^ 

yuyv2img : convyuyv.o yuyv.o
	$(CC) -o $@ $^ -lpng $(LDFLAGS) $(LDLIBS)

############################################################################
# For future reference
# To statically link one library (e.g. libgsl.a) and dyanmic for remainder
#	$(CC) ... $^ -Wl,-Bstatic -L$(HOME)/lib -lgsl -Wl,-Bdynamic -lm -lpthread
############################################################################

############################################################################
# Unit tests

x11video : video.c fourcc.c firstdev.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -DUNIT_TEST_VIDEO=1 -DHAVE_X11 -o $@ -lX11 -lXpm $^

snapshot : video.c fourcc.c firstdev.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -D_POSIX_C_SOURCE=200112L -DUNIT_TEST_VIDEO=1 -o $@ $^

############################################################################

clean : 
	rm -f *.o ut-* $(STATICLIB) $(DEPLOYMENT_ARCHIVE).tar.gz yuyv2img x11video snapshot

$(DEPLOYMENT_ARCHIVE).tar.gz : *.c *.h Makefile
	mkdir $(DEPLOYMENT_ARCHIVE)
	cp -L $^ "$(DEPLOYMENT_ARCHIVE)/"
	tar cfz $@ "$(DEPLOYMENT_ARCHIVE)/"
	rm -rf $(DEPLOYMENT_ARCHIVE)

.PHONY : all allunit clean upload 

