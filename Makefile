
STATICLIB=libvideo.a
BUILD_ARCH=$(shell uname -m)

ifdef DEBUG
CFLAGS+=-g -D_DEBUG
else
CFLAGS+=-O3 -DNDEBUG
endif

CFLAGS+=-Wall -std=c99
# c99 necessary to use the inline keyword.
# But note from https://gcc.gnu.org/onlinedocs/gcc/Inline.html#Inline:
#  GCC does not inline any functions when not optimizing unless you specify
#  the ‘always_inline’ attribute for the function, like this: 
#  // Prototype.
#  inline void foo (const char) __attribute__((always_inline));

CFLAGS+=-DHAVE_SINGLETON_MONITOR
# The resulting executable will only monitor one video device per process.

#CFLAGS += -D_POSIX_C_SOURCE=200809L
# _POSIX_SOURCE sufficient for localtime_r.
# getline requires _POSIX_C_SOURCE=200809L.

############################################################################

OBJECTS=video.o \
	fourcc.o \
	firstdev.o \
	convert.o \
	png.o

############################################################################
# Rules

all : $(STATICLIB)

# Following object dependencies use GNU make's implicit rules.

# Core modules.

video.o  : video.h vidfmt.h vidfrm.h fourcc.h

# Helper/accessory modules

fourcc.o   : fourcc.h
firstdev.o :
convert.o  :

yuv2pnm : yuyv2pnm.o yuyv2rgb.o pnm.o
	$(CC) -o $@ $^

$(STATICLIB) : $(OBJECTS)
	$(AR) rcs $@ $^ 

############################################################################
# For future reference
# To statically link one library (e.g. libgsl.a) and dyanmic for remainder
#	$(CC) ... $^ -Wl,-Bstatic -L$(HOME)/lib -lgsl -Wl,-Bdynamic -lm -lpthread
############################################################################

############################################################################
# Unit tests

UNITTESTS=$(addprefix ut-,video)

allunit : $(UNITTESTS)

ut-video : video.c fourcc.c firstdev.c
	$(CC) $(CFLAGS) -DUNIT_TEST_VIDEO=1 -o $@ -lX11 -lXpm $^

############################################################################

clean : 
	rm -f *.o ut-* $(STATICLIB) $(DEPLOYMENT_ARCHIVE).tar.gz yuv2pnm

veryclean : clean
	rm -f *.bin *.pgm

$(DEPLOYMENT_ARCHIVE).tar.gz : *.c *.h Makefile
	mkdir $(DEPLOYMENT_ARCHIVE)
	cp -L $^ "$(DEPLOYMENT_ARCHIVE)/"
	tar cfz $@ "$(DEPLOYMENT_ARCHIVE)/"
	rm -rf $(DEPLOYMENT_ARCHIVE)

upload : $(DEPLOYMENT_ARCHIVE).tar.gz
	scp $< pi@10.0.0.2:$<

.PHONY : all allunit clean veryclean upload 

