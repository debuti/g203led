default: all

g203led: g203led.c
	@gcc g203led.c -o g203led -lusb-1.0 -I/usr/include/libusb-1.0

all: g203led

clean:
	-@rm -f *.o
	-@rm -f g203led
