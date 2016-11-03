all:
	gcc diffrelax.c -o diffrelax @cflags

clean:
	rm diffrelax
