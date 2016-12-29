all: auto_brightness

auto_brightness: main.o camera.o backlight.o
	cc -o auto_brightness main.o camera.o backlight.o -lpthread

clean:
	-rm -f auto_brightness
	-rm -f *.o *~
