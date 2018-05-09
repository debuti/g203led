default: all

g203led: g203led.c
	@gcc g203led.c -o g203led -lusb

all: g203led

clean:
	-@rm -f *.o
	-@rm -f g203led
