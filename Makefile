ifdef ARCH
	ARCH_OPT = -march=$(ARCH)
endif

ifeq ($(BUILD),Release)
	COMMON_FLAGS = -pedantic -O3 -ffast-math $(ARCH_OPT)
else
	COMMON_FLAGS = -pedantic -Og -g
endif

COMMON_FLAGS+=-Wstrict-aliasing -Wall -Wextra -Iinclude -I.
CFLAGS=-std=c11 $(COMMON_FLAGS) $(DEFINES)
CXXFLAGS=-std=c++14 $(COMMON_FLAGS)
LDFLAGS=-lm -lpthread

TARGETS=fractalgen bezier range mandelbrot-double.so render-rgb.so

ifdef DO_ASM
SOURCES=$(wildcard *.c)
ASM_SOURCES=$(SOURCES:%.tpl.c=)
ASM_OBJECTS=$(ASM_SOURCES:.c=.s)
endif

%.s: %.c
	$(CC) $(CFLAGS) -S $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

all: $(TARGETS) $(ASM_OBJECTS) $(TESTS)

fractalgen: fractalgen.cpp bmp.o global.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) -ldl

drawtopleft: drawtopleft.o bmp.o
	$(CC) $(CFLAGS) $^ -o $@

mandelbrot-double.so: mandelbrot-itr-double.o
	$(CC) $(CFLAGS) $^ -o $@ -shared -fPIC

render-rgb.so: render-rgb.o
	$(CC) $(CFLAGS) $^ -o $@ -shared -fPIC

bezier: bezier.c

range: range.c

bezier-points: bezier
	./bezier 0 1 0 0 1 0 0 0 100 > $@

img.bmp: drawtopleft
	./drawtopleft > /dev/null

test-image: mandelbrot
	./mandelbrot -w 320 -h 240 -x 0 -y 0 -r 1.25 -a 1000 -t 4

imgdump: img.bmp
	tail -c +28 img.bmp | xxd -p -c $$((32 * 3 * 2)) > imgdump

clean:
	rm -f *.s *.o *.so $(TARGETS)
