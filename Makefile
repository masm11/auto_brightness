all: auto_brightness camera_test

CFLAGS = -g -Wall

auto_brightness: main.o camera.o backlight.o
	cc -o auto_brightness main.o camera.o backlight.o -lpthread

camera_test: camera_test.o
	cc -o camera_test camera_test.o -lX11

clean:
	-rm -f auto_brightness
	-rm -f *.o *~
